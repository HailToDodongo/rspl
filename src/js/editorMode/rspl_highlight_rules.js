/**
 * @TODO:
 * This is extremely hacky, use the proper method of defining custom syntax
 * even though it sucks.
 * 
 * Ref: 
 * https://github.com/ajaxorg/ace/blob/master/src/mode/glsl_highlight_rules.js
 */

import ace from "ace-builds";
import cpp from 'ace-builds/src-min-noconflict/mode-c_cpp';
const cppMode = new cpp.Mode();

var oop = {
  inherits: function(ctor, superCtor) {
    if(!superCtor)return;
    ctor.super_ = superCtor;
    ctor.prototype = Object.create(superCtor.prototype, {
        constructor: {
            value: ctor,
            enumerable: false,
            writable: true,
            configurable: true
        }
    });
}
};

const rsplHighlightRules = function() {
  const keywords =
        [
          "state", "function", "command",
          "if", "else", "for", "goto",

          "s8","u8","s16","u16","s32","u32",
          "vec16","vec32",

          "xxzzXXZZ", "yywwYYWW",
          "xxxxXXXX", "yyyyYYYY", "zzzzZZZZ", "wwwwWWWW",
          "xxxxxxxx", "yyyyyyyy", "zzzzzzzz", "wwwwwwww",
          "XXXXXXXX", "YYYYYYYY", "ZZZZZZZZ", "WWWWWWWW",
          "x", "y", "z", "w", "X", "Y", "Z", "W",

        ].join("|");

    const buildinConstants = [
    ].join("|")

    const variables = [
       "$v00" , "$v01" , "$v02" , "$v03" , "$v04" , "$v05" , "$v06" , "$v07" ,
        "$v08" , "$v09" , "$v10" , "$v11" , "$v12" , "$v13" , "$v14" , "$v15" ,
        "$v16" , "$v17" , "$v18" , "$v19" , "$v20" , "$v21" , "$v22" , "$v23" ,
        "$v24" , "$v25" , "$v26" , "$v27" , "$v28" , "$v29" , "$v30" , "$v31" ,
        "$at" , "$zero" , "$v0" , "$v1" , "$a0" , "$a1" , "$a2" , "$a3" ,
        "$t0" , "$t1" , "$t2" , "$t3" , "$t4" , "$t5" , "$t6" , "$t7" , "$t8" , "$t9" ,
        "$s0" , "$s1" , "$s2" , "$s3" , "$s4" , "$s5" , "$s6" , "$s7" ,
        "$k0" , "$k1" , "$gp" , "$sp" , "$fp" , "$ra",
    ].join("|");

    const functions = [
      "load", "store", "asm",
    ].join("|");

    var keywordMapper = this.createKeywordMapper({
        "support.function": functions,
        "variable.language": variables,
        "keyword": keywords,
        "constant.language": buildinConstants,
        "constant.language.boolean": "true|false"
    }, "identifier");

    this.$rules = new cppMode.HighlightRules().$rules;
    this.$rules.start.forEach(function(rule) {
        if (typeof rule.token == "function")
            rule.token = keywordMapper;
    });
};
console.log(cpp);

oop.inherits(rsplHighlightRules, cppMode.HighlightRules);

export default rsplHighlightRules;