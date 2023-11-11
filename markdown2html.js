import showdown from 'showdown';
import fs from 'fs';

const markdown = fs.readFileSync('./Docs.md', 'utf8');

const converter = new showdown.Converter();
const html = converter.makeHtml(markdown);

fs.writeFileSync('./src/web/docs.html', html);
