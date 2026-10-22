// A yt-dlp that answers a playlist look-up by the list id, as the real one does: a name, YouTube's errors for ids
// starting with PLmissing and PLprivate, a network error for PLbroken, and a name 1.5 s late for PLslow. Each run adds
// its arguments, joined with |, to yt-dlp-runs.txt beside it.

using System;
using System.IO;
using System.Text;
using System.Threading;

public static class FakeYtDlp {
	public static int Main(string[] args) {
		File.AppendAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "yt-dlp-runs.txt"), string.Join("|", args) + "\n", new UTF8Encoding(false));
		string url = args[args.Length - 1];
		int list = url.IndexOf("list=");
		string id = list < 0 ? "" : url.Substring(list + 5);
		if (id.StartsWith("PLslow")) Thread.Sleep(1500);
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
		Write(Console.OpenStandardOutput(), "{\"_type\": \"playlist\", \"id\": \"" + id + "\", \"title\": \"" + title + "\", \"entries\": []}\n");
		return 0;
	}

	static void Write(Stream stream, string text) {
		byte[] bytes = Encoding.UTF8.GetBytes(text);
		stream.Write(bytes, 0, bytes.Length);
		stream.Flush();
	}
}
