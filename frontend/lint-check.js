const { ESLint } = require('eslint');

async function main() {
  const eslint = new ESLint();
  const results = await eslint.lintFiles(['src/**/*.{ts,tsx}']);

  let errorCount = 0;
  let warningCount = 0;

  for (const result of results) {
    if (result.errorCount > 0 || result.warningCount > 0) {
      console.log(`\n${result.filePath}:`);
      for (const msg of result.messages) {
        const type = msg.severity === 2 ? 'ERROR' : 'WARN';
        console.log(`  [${type}] line ${msg.line}:${msg.column} - ${msg.message} (${msg.ruleId})`);
      }
    }
    errorCount += result.errorCount;
    warningCount += result.warningCount;
  }

  console.log(`\nESLint Summary: ${results.length} files scanned, ${errorCount} errors, ${warningCount} warnings.`);
  if (errorCount > 0) {
    process.exit(1);
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
