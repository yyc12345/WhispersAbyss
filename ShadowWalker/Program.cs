using System;

namespace ShadowWalker {
    class Program {
        static void Main(string[] args) {
            Console.WriteLine("Shadow Walker - A stup*d BMMO Bot.");
            Console.WriteLine("");
            Console.WriteLine("The world remains constant over the centuries.");
            Console.WriteLine("But human life is like the dew at dawn or a bubble rising through water.");
            Console.WriteLine("Transitory.");
            Console.WriteLine("");
            Console.WriteLine("======");

            Kernel.OutputHelper outputHelper = new Kernel.OutputHelper();
            Kernel.CelestiaClient celestiaClient = new Kernel.CelestiaClient(outputHelper, "127.0.0.1", "6172");
            Kernel.DataProcessor dataProcessor = new Kernel.DataProcessor(celestiaClient, outputHelper);

            bool quit;
            while (true) {
                var key = Console.ReadKey(true);
                if (key.Key == ConsoleKey.Tab) {
                    var cmd = outputHelper.GetCommand();
                    dataProcessor.ProcServerCommand(cmd, out quit);
                    if (quit) break;
                } else if (key.Key == ConsoleKey.Q) {
                    break;
                }
            }

            dataProcessor.Dispose();
            outputHelper.Printf("Detached from the world...");

        }
    }
}
