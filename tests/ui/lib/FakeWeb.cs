// A local stand-in for open.spotify.com, and for GitHub's API when it has a yt-dlp to serve, so the checks never reach
// the network. Embed pages for the ids in SpotifyNames hold that name; other ids get Spotify's error page.

using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

public class FakeWeb {
	readonly TcpListener listener = new TcpListener(IPAddress.Loopback, 0);
	public readonly Dictionary<string, string> SpotifyNames = new Dictionary<string, string>();
	public readonly HashSet<string> SlowIds = new HashSet<string>();
	public readonly HashSet<string> BrokenIds = new HashSet<string>();
	// The yt-dlp download waits for it, so a check can look at the app while yt-dlp installs
	public readonly ManualResetEvent AssetGate = new ManualResetEvent(false);
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
		listener.Stop();
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
				while (!string.IsNullOrEmpty(reader.ReadLine())) { }
				int status = 200;
				byte[] body = Answer(path, ref status);
				byte[] head = Encoding.ASCII.GetBytes("HTTP/1.1 " + status + " Fake\r\nContent-Length: " + body.Length + "\r\nConnection: close\r\n\r\n");
				stream.Write(head, 0, head.Length);
				stream.Write(body, 0, body.Length);
			}
		} catch { }
	}

	byte[] Answer(string path, ref int status) {
		const string Embed = "/embed/playlist/";
		if (path.StartsWith(Embed)) {
			string id = path.Substring(Embed.Length);
			if (SlowIds.Contains(id)) Thread.Sleep(1500);
			if (BrokenIds.Contains(id)) { status = 500; return new byte[0]; }
			string name;
			string data = SpotifyNames.TryGetValue(id, out name)
				? "{\"props\":{\"pageProps\":{\"state\":{\"data\":{\"entity\":{\"name\":\"" + name + "\",\"type\":\"playlist\"}}}}}}"
				: "{\"props\":{\"pageProps\":{\"status\":500,\"title\":\"Page not available\"}}}";
			return Encoding.UTF8.GetBytes("<html><body><script id=\"__NEXT_DATA__\" type=\"application/json\">" + data + "</script></body></html>");
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
}
