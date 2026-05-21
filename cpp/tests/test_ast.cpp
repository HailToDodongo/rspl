#include <catch2/catch_test_macros.hpp>
#include "ast.h"

using namespace rspl::ast;

TEST_CASE("parse minimal function", "[ast]") {
  auto prog = parseJson(R"({
    "includes": [],
    "states": [],
    "functions": [{
      "annotations": [],
      "type": "function",
      "resultType": null,
      "name": "test",
      "args": [],
      "body": {
        "type": "scopedBlock",
        "statements": [{
          "type": "varDeclMulti",
          "varType": "u32",
          "reg": "$t0",
          "varNames": ["a","b","c"],
          "isConst": false,
          "line": 2
        }],
        "line": 1
      }
    }],
    "postIncludes": []
  })");

  REQUIRE(prog.functions.size() == 1);
  REQUIRE(prog.functions[0].name == "test");
  REQUIRE(toString(prog.functions[0].type) == "function");
  REQUIRE(prog.functions[0].body != nullptr);
  REQUIRE(prog.functions[0].body->statements.size() == 1);
}

TEST_CASE("parse calcNum with plain number", "[ast]") {
  auto prog = parseJson(R"({
    "includes": [], "states": [],
    "functions": [{
      "annotations": [], "type": "function", "resultType": null, "name": "f", "args": [],
      "body": {
        "type": "scopedBlock", "statements": [{
          "type": "varDeclAssign",
          "varType": "u32", "varName": "x",
          "calc": { "type": "calcNum", "right": 42 },
          "isConst": false, "line": 1
        }], "line": 1
      }
    }],
    "postIncludes": []
  })");
  REQUIRE(prog.functions.size() == 1);
}

TEST_CASE("parse compare in if statement", "[ast]") {
  auto prog = parseJson(R"({
    "includes": [], "states": [],
    "functions": [{
      "annotations": [], "type": "function", "resultType": null, "name": "f", "args": [],
      "body": {
        "type": "scopedBlock", "statements": [{
          "type": "if",
          "compare": {
            "left": {"type":"var","value":"a"},
            "op": ">",
            "right": {"type":"num","value":10}
          },
          "blockIf": {
            "type": "scopedBlock", "statements": [{
              "type": "varAssignCalc",
              "varName": "b",
              "assignType": "=",
              "calc": { "type": "calcNum", "right": 5 },
              "line": 2
            }], "line": 2
          },
          "blockElse": null,
          "line": 1
        }], "line": 1
      }
    }],
    "postIncludes": []
  })");
  REQUIRE(prog.functions.size() == 1);
}
