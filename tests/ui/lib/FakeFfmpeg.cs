// An ffmpeg that copies its input (-i) to its output (the last argument), as the cover's crop needs nothing more. Each
// run adds its arguments, joined with |, to ffmpeg-runs.txt beside it.

using System;
using System.IO;
using System.Text;

public static class FakeFfmpeg {
	public static int Main(string[] args) {
		string runs = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "ffmpeg-runs.txt");
		// Downloads run side by side, so another run may hold the file a moment
		for (int tries = 0; ; tries++) {
			try {
				File.AppendAllText(runs, string.Join("|", args) + "\n", new UTF8Encoding(false));
				break;
			} catch (IOException) {
				if (tries >= 50) throw;
				System.Threading.Thread.Sleep(20);
			}
		}
		int input = Array.IndexOf(args, "-i");
		if (input < 0 || input + 1 >= args.Length) return 1;
		File.Copy(args[input + 1], args[args.Length - 1], true);
		return 0;
	}
}
