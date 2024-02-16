/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/

/**
 * Runs a C-like preprocess on the source code.
 * This handles constants as well as ifdefs.
 * @param src input source code
 * @return {string} processed source code
 */
export function preprocess(src)
{
  const lines = src.split("\n");
  const defines = {};

  let srcRes = "";
  let insideIfdef = false;
  let ignoreLine = false;

  for (let i = 0; i < lines.length; i++)
  {
    let line = lines[i];
    const lineTrimmed = line.trim();
    let newLine = "";

    if (lineTrimmed.startsWith("#define"))
    {
      const parts = lineTrimmed.match(/#define\s+([a-zA-Z0-9_]+)\s+(.*)/);
      if(!parts)throw new Error(`Line ${i+1}: Invalid #define statement!`);
      const [_, name, value] = parts;

      defines[name] = {
        regex: new RegExp(`\\b${name}\\b`, "g"),
        value
      };
    }
    else if (lineTrimmed.startsWith("#undef"))
    {
      const parts = lineTrimmed.match(/#undef\s+([a-zA-Z0-9_]+)/);
      if(!parts)throw new Error(`Line ${i+1}: Invalid #undef statement!`);
      const [_, name] = parts;

      delete defines[name];
    }
    else if (lineTrimmed.startsWith("#ifdef"))
    {
      if(insideIfdef)throw new Error(`Line ${i+1}: Nested #ifdef statements are not allowed!`);
      insideIfdef = true;
      const parts = lineTrimmed.match(/#ifdef\s+([a-zA-Z0-9_]+)/);
      if(!parts)throw new Error(`Line ${i+1}: Invalid #ifdef statement!`);
      const [_, name] = parts;

      ignoreLine = !defines[name];
    }
    else if (lineTrimmed.startsWith("#else")) {
      ignoreLine = insideIfdef && !ignoreLine;
    }
    else if (lineTrimmed.startsWith("#endif")) {
      insideIfdef = false;
      ignoreLine = false;
    }
    else if (lineTrimmed.startsWith("#include"))
    {
      // Ignore for now
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
