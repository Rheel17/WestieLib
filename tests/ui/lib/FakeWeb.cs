// A local stand-in for open.spotify.com and Spotify's query API, and for GitHub's API when it has a yt-dlp to serve, so
// the checks never reach the network. Embed pages for the ids in SpotifyNames hold that name and an access token; other
// ids get Spotify's error page. The query API answers with the playlist's SpotifyItems, a page at a time, when it gets
// that token; with Spotify's errors for ErrorQueryIds; and with NotFound for ids without items. Each query is logged in
// SpotifyQueries, the pages after the first wait for SpotifyPageGate, and SpotifyFailures makes pages fail with HTTP 500.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

public class FakeWeb {
	readonly TcpListener listener = new TcpListener(IPAddress.Loopback, 0);
	readonly List<string> spotifyQueries = new List<string>();
	public readonly Dictionary<string, string> SpotifyNames = new Dictionary<string, string>();
	public readonly Dictionary<string, List<string>> SpotifyItems = new Dictionary<string, List<string>>();
	public readonly HashSet<string> SlowIds = new HashSet<string>();
	public readonly HashSet<string> BrokenIds = new HashSet<string>();
	public readonly HashSet<string> ErrorQueryIds = new HashSet<string>();
	// How many more times the query for "<playlist id> <offset>" gets HTTP 500
	public readonly Dictionary<string, int> SpotifyFailures = new Dictionary<string, int>();
	// The yt-dlp download waits for it, so a check can look at the app while yt-dlp installs
	public readonly ManualResetEvent AssetGate = new ManualResetEvent(false);
	// Every page of a Spotify query after the first waits for it, so a check can look at the app between pages
	public readonly ManualResetEvent SpotifyPageGate = new ManualResetEvent(true);
	public byte[] YtDlp;
	public int Port;
	public string Url { get { return "http://127.0.0.1:" + Port; } }

	public FakeWeb() {
		listener.Start();
		Port = ((IPEndPoint)listener.LocalEndpoint).Port;
		var thread = new Thread(Accept);
		thread.IsBackground = true;
		thread.Start();
	}

	public void Stop() {
		AssetGate.Set();
		SpotifyPageGate.Set();
		listener.Stop();
	}

	// Each query as "<playlist id> <offset> <limit> <Authorization> <app-platform>"
	public string[] SpotifyQueries() {
		lock (spotifyQueries) return spotifyQueries.ToArray();
	}

	// Playlist items as the query API has them, with fields the app doesn't read too
	public static string Track(string id, string title, string[] artists, string album, long milliseconds) {
		string artistItems = string.Join(",", artists.Select(a => "{\"profile\":{\"name\":" + Json(a) + "},\"uri\":\"spotify:artist:" + id + "\"}"));
		return "{\"addedAt\":{\"isoString\":\"2026-07-09T14:06:14.326Z\"},\"addedBy\":null,\"itemV2\":{\"__typename\":\"TrackResponseWrapper\",\"data\":{" +
			"\"__typename\":\"Track\",\"albumOfTrack\":{\"artists\":{\"items\":[" + artistItems + "]},\"coverArt\":{\"sources\":[{\"height\":640," +
			"\"url\":\"https://i.scdn.co/image/" + id + "\",\"width\":640}]},\"name\":" + Json(album) + ",\"uri\":\"spotify:album:" + id + "\"}," +
			"\"artists\":{\"items\":[" + artistItems + "]},\"discNumber\":1,\"trackDuration\":{\"totalMilliseconds\":" + milliseconds + "}," +
			"\"name\":" + Json(title) + ",\"playability\":{\"playable\":true,\"reason\":\"PLAYABLE\"},\"playcount\":\"1234\",\"trackNumber\":1," +
			"\"uri\":\"spotify:track:" + id + "\"}}}";
	}

	public static string Episode(string id, string title) {
		return "{\"addedAt\":{\"isoString\":\"2026-07-09T14:06:14.326Z\"},\"itemV2\":{\"__typename\":\"EpisodeResponseWrapper\",\"data\":{" +
			"\"__typename\":\"Episode\",\"name\":" + Json(title) + ",\"uri\":\"spotify:episode:" + id + "\",\"duration\":{\"totalMilliseconds\":1800000}," +
			"\"podcastV2\":{\"data\":{\"name\":\"A Podcast\"}},\"artists\":null}}}";
	}

	public static string LocalFile(string title) {
		return "{\"addedAt\":{\"isoString\":\"2026-07-09T14:06:14.326Z\"},\"itemV2\":{\"__typename\":\"LocalTrackResponseWrapper\",\"data\":{" +
			"\"__typename\":\"LocalTrack\",\"name\":" + Json(title) + ",\"uri\":\"spotify:local:Someone::" + Uri.EscapeDataString(title) + ":200\"," +
			"\"trackDuration\":{\"totalMilliseconds\":200000}}}}";
	}

	static string Json(string text) {
		return "\"" + text.Replace("\\", "\\\\").Replace("\"", "\\\"") + "\"";
	}

	void Accept() {
		while (true) {
			TcpClient client;
			try { client = listener.AcceptTcpClient(); } catch { return; }
			ThreadPool.QueueUserWorkItem(_ => Serve(client));
		}
	}

	void Serve(TcpClient client) {
		try {
			using (client)
			using (var stream = client.GetStream()) {
				var reader = new StreamReader(stream, Encoding.ASCII);
				string path = reader.ReadLine().Split(' ')[1];
				var headers = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
				string line;
				while (!string.IsNullOrEmpty(line = reader.ReadLine())) {
					int colon = line.IndexOf(':');
					if (colon > 0) headers[line.Substring(0, colon).Trim()] = line.Substring(colon + 1).Trim();
				}
				int status = 200;
				byte[] body = Answer(path, headers, ref status);
				byte[] head = Encoding.ASCII.GetBytes("HTTP/1.1 " + status + " Fake\r\nContent-Length: " + body.Length + "\r\nConnection: close\r\n\r\n");
				stream.Write(head, 0, head.Length);
				stream.Write(body, 0, body.Length);
			}
		} catch { }
	}

	byte[] Answer(string path, Dictionary<string, string> headers, ref int status) {
		const string Embed = "/embed/playlist/";
		const string Query = "/pathfinder/v1/query?";
		if (path.StartsWith(Embed)) {
			string id = path.Substring(Embed.Length);
			if (SlowIds.Contains(id)) Thread.Sleep(1500);
			if (BrokenIds.Contains(id)) { status = 500; return new byte[0]; }
			string name;
			string data = SpotifyNames.TryGetValue(id, out name)
				? "{\"props\":{\"pageProps\":{\"state\":{\"data\":{\"entity\":{\"name\":" + Json(name) + ",\"type\":\"playlist\"}},\"settings\":{\"session\":{" +
					"\"accessToken\":\"fake-token-" + id + "\",\"accessTokenExpirationTimestampMs\":1792757766234,\"isAnonymous\":true}}}}}}"
				: "{\"props\":{\"pageProps\":{\"status\":500,\"title\":\"Page not available\"}}}";
			return Encoding.UTF8.GetBytes("<html><body><script id=\"__NEXT_DATA__\" type=\"application/json\">" + data + "</script></body></html>");
		}
		if (path.StartsWith(Query)) {
			return Encoding.UTF8.GetBytes(QueryAnswer(path.Substring(Query.Length), headers, ref status));
		}
		if (path == "/repos/yt-dlp/yt-dlp/releases/latest" && YtDlp != null) {
			string digest;
			using (var sha = SHA256.Create()) digest = BitConverter.ToString(sha.ComputeHash(YtDlp)).Replace("-", "").ToLowerInvariant();
			return Encoding.UTF8.GetBytes("{\"tag_name\":\"2026.10.22\",\"assets\":[{\"name\":\"yt-dlp.exe\",\"browser_download_url\":\"" + Url +
				"/download/yt-dlp.exe\",\"size\":" + YtDlp.Length + ",\"digest\":\"sha256:" + digest + "\"}]}");
		}
		if (path == "/download/yt-dlp.exe" && YtDlp != null) {
			AssetGate.WaitOne();
			return YtDlp;
		}
		status = 404;
		return new byte[0];
	}

	string QueryAnswer(string query, Dictionary<string, string> headers, ref int status) {
		var parameters = query.Split('&').Select(p => p.Split(new[] { '=' }, 2)).Where(p => p.Length == 2)
			.ToDictionary(p => p[0], p => Uri.UnescapeDataString(p[1]));
		string variables;
		parameters.TryGetValue("variables", out variables);
		variables = variables ?? "";
		string id = Regex.Match(variables, "\"uri\":\"spotify:playlist:([A-Za-z0-9]+)\"").Groups[1].Value;
		int offset = int.Parse("0" + Regex.Match(variables, "\"offset\":(\\d+)").Groups[1].Value);
		int limit = int.Parse("0" + Regex.Match(variables, "\"limit\":(\\d+)").Groups[1].Value);
		string authorization, platform;
		headers.TryGetValue("Authorization", out authorization);
		headers.TryGetValue("app-platform", out platform);
		lock (spotifyQueries) spotifyQueries.Add(id + " " + offset + " " + limit + " " + authorization + " " + platform);
		if (offset > 0) SpotifyPageGate.WaitOne();

		string operation;
		parameters.TryGetValue("operationName", out operation);
		if (operation != "fetchPlaylist" || !variables.Contains("\"enableWatchFeedEntrypoint\":false")) { status = 400; return ""; }
		if (authorization != "Bearer fake-token-" + id) { status = 401; return ""; }
		lock (SpotifyFailures) {
			int failures;
			string failing = id + " " + offset;
			if (SpotifyFailures.TryGetValue(failing, out failures) && failures > 0) {
				SpotifyFailures[failing] = failures - 1;
				status = 500;
				return "";
			}
		}
		if (ErrorQueryIds.Contains(id)) return "{\"errors\":[{\"message\":\"PersistedQueryNotFound\"}]}";
		List<string> items;
		if (!SpotifyItems.TryGetValue(id, out items)) {
			return "{\"data\":{\"playlistV2\":{\"__typename\":\"NotFound\",\"message\":\"Object with uri 'spotify:playlist:" + id + "' not found\"}}}";
		}
		string page = string.Join(",", items.Skip(offset).Take(limit));
		return "{\"data\":{\"playlistV2\":{\"__typename\":\"Playlist\",\"name\":\"Fake\",\"content\":{\"__typename\":\"PlaylistItemsPage\",\"items\":[" + page +
			"],\"pagingInfo\":{\"limit\":" + limit + ",\"offset\":" + offset + "},\"totalCount\":" + items.Count + "}}}}";
	}
}
