import showdown from 'showdown';
import fs from 'fs';

const markdown = fs.readFileSync('./Docs.md', 'utf8');

const converter = new showdown.Converter();
const html = converter.makeHtml(markdown);

let mainHtml = fs.readFileSync('./src/web/index.html', 'utf8');

const offsetStart = mainHtml.indexOf('RSPL_DOCS">') + 'RSPL_DOCS">'.length;
const offsetEnd = mainHtml.indexOf('</div>', offsetStart);
mainHtml = mainHtml.substring(0, offsetStart) + html + mainHtml.substring(offsetEnd);
fs.writeFileSync('./src/web/index.html', mainHtml);
