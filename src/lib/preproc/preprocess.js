/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/

import state from "../state.js";

export function stripComments(source) {
  return source
    .replaceAll(/\/\/.*$/gm, "")
    .replace(/\/\*[\s\S]*?\*\//g, match => {
      const newlineCount = match.split('\n').length - 1;
      return '\n'.repeat(newlineCount);
    });
}

/**
 * Runs a C-like preprocess on the source code.
 * This handles constants as well as ifdefs.
 * @param src input source code
 * @param defines this function will append all defines to this object
 * @param {(string) => string} fileLoader function to load included files
 * @return {string} processed source code
 */
export function preprocess(src, defines = {}, fileLoader = undefined)
{
  const lines = src.split("\n");

  let srcRes = "";
  let insideIfdef = false;
  let ignoreLine = false;

  for (let i = 0; i < lines.length; i++)
  {
    let line = lines[i];
    const lineTrimmed = line.trim();
    let newLine = "";
    state.func = "preprocessor";
    state.line = i + 1;

    if (!ignoreLine && lineTrimmed.startsWith("#define"))
    {
      const parts = lineTrimmed.match(/#define\s+([a-zA-Z0-9_]+)\s+(.*)/);
      if(!parts)throw new Error(`Line ${i+1}: Invalid #define statement!`);
      let [_, name, value] = parts;

      for (const data of Object.values(defines))  {
        value = value.replace(data.regex, data.value);
      }

      defines[name] = {
        regex: new RegExp(`\\b${name}\\b`, "g"),
        value
      };
    }
    else if (!ignoreLine && lineTrimmed.startsWith("#undef"))
    {
      const parts = lineTrimmed.match(/#undef\s+([a-zA-Z0-9_]+)/);
      if(!parts)throw new Error(`Line ${i+1}: Invalid #undef statement!`);
      const [_, name] = parts;

      delete defines[name];
    }
    else if (lineTrimmed.startsWith("#ifdef") || lineTrimmed.startsWith("#ifndef"))
    {
      if(insideIfdef)throw new Error(`Line ${i+1}: Nested #ifdef statements are not allowed!`);
      insideIfdef = true;
      const parts = lineTrimmed.match(/#ifn?def\s+([a-zA-Z0-9_]+)/);
      if(!parts)throw new Error(`Line ${i+1}: Invalid #ifdef statement!`);
      const [_, name] = parts;

      const negate = lineTrimmed.startsWith("#ifdef");
      ignoreLine = negate ? !defines[name] : defines[name];
    }
    else if (lineTrimmed.startsWith("#else")) {
      ignoreLine = insideIfdef && !ignoreLine;
    }
    else if (lineTrimmed.startsWith("#endif")) {
      insideIfdef = false;
      ignoreLine = false;
    }
    else if (!ignoreLine && lineTrimmed.startsWith("#include"))
    {
      if(!fileLoader) {
        state.throwError(`Line ${i+1}: #include statement requires a fileLoader function!`);
      }
      const filePath = lineTrimmed.match(/#include\s+"(.*)"/)[1];
      const fileContent = fileLoader(filePath);
      srcRes += preprocess(stripComments(fileContent), defines, fileLoader);

    } else if(!ignoreLine) {
      // replace all defines
      newLine = line;
      for (const data of Object.values(defines))  {
        newLine = newLine.replace(data.regex, data.value);
      }
    }

    srcRes += newLine + "\n";
  }

  return srcRes;
}
