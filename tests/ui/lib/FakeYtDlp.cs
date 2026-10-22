// A yt-dlp that answers playlists by the list id, as the real one does. A look-up (--playlist-items 0) gets a name; a
// fetch gets the entries too: songs for ids starting with PLtracks and PLshared (which share one video), none for any
// other id. Ids starting with PLmissing and PLprivate get YouTube's errors, PLbroken a network error, and PLslow its
// answer 1.5 s late (a fetch 3 s late). Each run adds its arguments, joined with |, to yt-dlp-runs.txt beside it.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;

public static class FakeYtDlp {
	public static int Main(string[] args) {
		File.AppendAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "yt-dlp-runs.txt"), string.Join("|", args) + "\n", new UTF8Encoding(false));
		bool lookUp = Array.IndexOf(args, "--playlist-items") >= 0;
		string url = args[args.Length - 1];
		int list = url.IndexOf("list=");
		string id = list < 0 ? "" : url.Substring(list + 5);
		if (id.StartsWith("PLslow")) Thread.Sleep(lookUp ? 1500 : 3000);
		string error = null;
		if (id.StartsWith("PLmissing")) error = "YouTube said: The playlist does not exist.";
		if (id.StartsWith("PLprivate")) error = "This playlist is private";
		if (id.StartsWith("PLbroken")) error = "Unable to download API page: HTTP Error 503: Service Unavailable";
		if (error != null) {
			Write(Console.OpenStandardError(), "WARNING: [youtube:tab] YouTube Music is not directly supported\nERROR: [youtube:tab] " + id + ": " + error + "\n");
			return 1;
		}
		// The real one escapes everything outside ASCII
		string title = id == "PLx9k2_-Ab" ? "Caf\\u00e9 Check Mix" : "Checked " + id;
		string entries = lookUp ? "" : string.Join(", ", Entries(id));
		Write(Console.OpenStandardOutput(), "{\"_type\": \"playlist\", \"id\": \"" + id + "\", \"title\": \"" + title + "\", \"entries\": [" + entries + "]}\n");
		return 0;
	}

	static IEnumerable<string> Entries(string id) {
		if (id.StartsWith("PLtracks")) {
			yield return Entry("trackAAAAA1", "\"Way Down We Go\"", "\"KALEO\"", "214");
			yield return Entry("trackBBBBB2", "\"Caf\\u00e9 Lights\"", "\"Nova - Topic\"", "191.0");
			yield return Entry("trackGONE03", "\"[Private video]\"", "null", "null");
			yield return Entry("trackCCCCC4", "\"No Length\"", "\"Somebody\"", "null");
			yield return Entry("trackSHARE5", "\"Shared Song\"", "\"Both\"", "200");
			yield return Entry("trackAAAAA1", "\"Way Down We Go\"", "\"KALEO\"", "214");
		}
		if (id.StartsWith("PLshared")) {
			yield return Entry("trackSHARE5", "\"Shared Song\"", "\"Both\"", "200");
			yield return Entry("trackDDDDD6", "\"Only Here\"", "\"Solo\"", "65");
		}
		if (id.StartsWith("PLslow")) {
			yield return Entry("trackSLOWW7", "\"Slow One\"", "\"Turtle\"", "300");
		}
	}

	// Like the real one's flat entries, with the fields the app doesn't read too
	static string Entry(string video, string title, string channel, string duration) {
		return "{\"_type\": \"url\", \"ie_key\": \"Youtube\", \"id\": \"" + video + "\", \"url\": \"https://music.youtube.com/watch?v=" + video +
			"\", \"title\": " + title + ", \"duration\": " + duration + ", \"channel\": " + channel +
			", \"thumbnails\": [{\"url\": \"https://i.ytimg.com/vi/" + video + "/hqdefault.jpg\", \"height\": 94, \"width\": 168}]}";
	}

	static void Write(Stream stream, string text) {
		byte[] bytes = Encoding.UTF8.GetBytes(text);
		stream.Write(bytes, 0, bytes.Length);
		stream.Flush();
	}
}
