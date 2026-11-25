// A yt-dlp that answers playlists by the list id, as the real one does. A look-up (--playlist-items 0) gets a name; a
// fetch gets the entries too: songs for ids starting with PLtracks and PLshared (which share one id), none for any
// other id. Ids starting with PLmissing and PLprivate get YouTube's errors, PLbroken a network error, and PLslow its
// answer 1.5 s late (a fetch 3 s late). A download (-x) prints progress and writes the audio, the info JSON and the
// thumbnail from the fixtures in fake-fixtures beside it; ids starting with bad, and those fail-ids.txt beside it
// lists, fail, and one with a hold-<id> file
// beside it stays at 50% while the file is there. Each run adds its arguments, joined with |, to yt-dlp-runs.txt
// beside it.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;

public static class FakeYtDlp {
	public static int Main(string[] args) {
		LogRun(args);
		if (Array.IndexOf(args, "-x") >= 0) return Download(args);
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

	// Downloads run side by side, so another run may hold the file a moment
	static void LogRun(string[] args) {
		string runs = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "yt-dlp-runs.txt");
		for (int tries = 0; ; tries++) {
			try {
				File.AppendAllText(runs, string.Join("|", args) + "\n", new UTF8Encoding(false));
				return;
			} catch (IOException) {
				if (tries >= 50) throw;
				Thread.Sleep(20);
			}
		}
	}

	// A progress line as the real one prints it: the template's "download:" only says which progress it is for, so it
	// doesn't show; without a template, its own line
	static string Progress(string[] args, string percent) {
		string template = ArgumentAfter(args, "--progress-template");
		if (template == "") return "[download] " + percent + " of    3.42MiB at  1.20MiB/s ETA 00:02\n";
		if (template.StartsWith("download:")) template = template.Substring("download:".Length);
		return template.Replace("%(progress._percent_str)s", percent) + "\n";
	}

	static string ArgumentAfter(string[] args, string name) {
		int index = Array.IndexOf(args, name);
		return index >= 0 && index + 1 < args.Length ? args[index + 1] : "";
	}

	static int Download(string[] args) {
		string url = args[args.Length - 1];
		string id = url.Substring(url.IndexOf("v=") + 2);
		string failing = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "fail-ids.txt");
		if (id.StartsWith("bad") || (File.Exists(failing) && Array.IndexOf(File.ReadAllLines(failing), id) >= 0)) {
			Write(Console.OpenStandardError(), "WARNING: [youtube] " + id + ": Some formats may be missing\nERROR: [youtube] " + id + ": Video unavailable\n");
			return 1;
		}
		string format = ArgumentAfter(args, "--audio-format");
		string folder = Path.GetDirectoryName(ArgumentAfter(args, "-o"));
		string here = AppDomain.CurrentDomain.BaseDirectory;
		string fixtures = Path.Combine(here, "fake-fixtures");
		Stream output = Console.OpenStandardOutput();
		Write(output, "[youtube] Extracting URL: " + url + "\n" + Progress(args, "  0.0%") + Progress(args, " 50.0%"));
		while (File.Exists(Path.Combine(here, "hold-" + id))) Thread.Sleep(100);
		Write(output, Progress(args, "100.0%") + "[ExtractAudio] Destination: " + Path.Combine(folder, id + "." + format) + "\n");
		File.Copy(Path.Combine(fixtures, "silence." + format), Path.Combine(folder, id + "." + format), true);
		File.Copy(Path.Combine(fixtures, "thumbnail.jpg"), Path.Combine(folder, id + ".jpg"), true);
		// Only trackGOOD01 has an album and a year, as many videos have neither
		string album = id == "trackGOOD01" ? "\"Good Album\", \"release_year\": 2021" : "null, \"release_year\": null";
		File.WriteAllText(Path.Combine(folder, id + ".info.json"),
			"{\"id\": \"" + id + "\", \"title\": \"Video " + id + "\", \"album\": " + album + ", \"formats\": [{\"format_id\": \"251\"}]}", new UTF8Encoding(false));
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
		// Downloads: one that fails, two that want the same file name, one with characters a file name can't have
		if (id.StartsWith("PLdownload")) {
			yield return Entry("trackGOOD01", "\"Good One\"", "\"Band - Topic\"", "200");
			yield return Entry("badTRACK002", "\"Gone Song\"", "\"Band\"", "180");
			yield return Entry("trackTWIN03", "\"Twin\"", "\"Pair\"", "150");
			yield return Entry("trackTWIN04", "\"Twin\"", "\"Pair\"", "151");
			yield return Entry("trackSLASH5", "\"A/B: Test?\"", "\"Who\"", "100");
		}
		if (id.StartsWith("PLflac")) {
			yield return Entry("trackFLAC06", "\"Lossless\"", "\"Flac\"", "120");
		}
		if (id.StartsWith("PLretry")) {
			yield return Entry("trackRETRY7", "\"Again\"", "\"Retrier\"", "140");
		}
		if (id.StartsWith("PLhold")) {
			yield return Entry("trackHOLD08", "\"Held\"", "\"Slowpoke\"", "130");
		}
		if (id.StartsWith("PLslow")) {
			yield return Entry("trackSLOWW7", "\"Slow One\"", "\"Turtle\"", "300");
		}
	}

	// Like the real one's flat entries, with the fields the app doesn't read too
	static string Entry(string id, string title, string channel, string duration) {
		return "{\"_type\": \"url\", \"ie_key\": \"Youtube\", \"id\": \"" + id + "\", \"url\": \"https://music.youtube.com/watch?v=" + id +
			"\", \"title\": " + title + ", \"duration\": " + duration + ", \"channel\": " + channel +
			", \"thumbnails\": [{\"url\": \"https://i.ytimg.com/vi/" + id + "/hqdefault.jpg\", \"height\": 94, \"width\": 168}]}";
	}

	static void Write(Stream stream, string text) {
		byte[] bytes = Encoding.UTF8.GetBytes(text);
		stream.Write(bytes, 0, bytes.Length);
		stream.Flush();
	}
}
