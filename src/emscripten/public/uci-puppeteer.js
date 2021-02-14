const process = require("process");
const os = require("os");
const puppeteer = require("puppeteer");

const IS_ROOT = os.userInfo().uid === 0;
const TEST_URL =
  process.env.TEST_URL ?? "http://localhost:5000/test-puppeteer.html";
const NO_LOG = !!process.env.NO_LOG;

function logger(...args) {
  if (NO_LOG) {
    return;
  }
  console.error("[NODE-LOG]", ...args);
}

async function pageScript(argv) {
  async function runRepl(stockfish) {
    console.log("TODO: REPL not implemented yet");
    stockfish.postMessage("quit");
  }

  return new Promise(async (resolve) => {
    const stockfish = await Stockfish({ onExit: resolve });
    if (argv.length > 0) {
      const commands = argv.join(" ").split("++");
      for (const command of commands) {
        stockfish.postMessage(command);
      }
      stockfish.postMessage("quit");
      return;
    }
    await runRepl(stockfish);
  });
}

async function main(argv) {
  logger("Launching browser");
  const browser = await puppeteer.launch({
    args: IS_ROOT ? ["--no-sandbox"] : [],
  });
  const page = await browser.newPage();
  page.on("console", (message) => console.log(message.text()));

  logger(`Navigating to ${TEST_URL}`);
  await page.goto(TEST_URL, { waitUntil: "networkidle2", timeout: 10000 });

  logger(`Running script`);
  await page.evaluate(pageScript, argv);

  logger(`Closing browser`);
  await browser.close();
}

if (require.main === module) {
  main(process.argv.slice(2));
}
