// Converte relatorio/relatorio.html em PDF usando o Chromium do Playwright.
// Uso: node relatorio/html_para_pdf.js relatorio/relatorio.html relatorio/relatorio.pdf
const path = require('path');
const { chromium } = require('playwright');

(async () => {
  const [entrada, saida] = process.argv.slice(2);
  const browser = await chromium.launch();
  const page = await browser.newPage();
  await page.goto('file://' + path.resolve(entrada), { waitUntil: 'networkidle' });
  await page.pdf({
    path: saida,
    format: 'A4',
    printBackground: true,
    preferCSSPageSize: true,
    displayHeaderFooter: true,
    headerTemplate: '<span></span>',
    footerTemplate:
      '<div style="width:100%;font-size:8px;color:#777;text-align:center;font-family:sans-serif">' +
      'Exercício Final — OpenMP · página <span class="pageNumber"></span> de <span class="totalPages"></span></div>',
  });
  await browser.close();
  console.log('PDF gerado em ' + saida);
})();
