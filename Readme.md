# RSPL - Transpiler

RSPL is a high-level language which can be transpiled into RSP MIPS assembly in text form.<br>
The output is a completely functional overlay usable by libdragon.

It's intended to be a simple language staying close to the hardware, while proving a GLSL-like "swizzle" syntax to make use of the vector/lane instructions on the RSP.

For a detailed documentation on the langue itself, see the language docs: [RSPL](Docs.md)

## Using RSPL

This project provides both a CLI, and an interactive Web-App which can transpile code in real-time.

A hosted version of the web-app is available at: https://mbeboek.gitlab.io/rspl/ <br/>

The CLI is currently not available as a pre-build, and needs to be build from sources.<br/>
See the build section for more information.

## Building
The only system requirement is [NodeJS](https://nodejs.org/), using version 20.0 or higher.<br/>

After checking out the repo, install all NPM packages with:
```sh
yarn install
```
This only needs to be done once, or when dependencies have changed.

### Web-App
The webapp can be both build, and started in development mode (which should be preferred when working on the transpiler).<br/>
To start it, run:
```sh
yarn start
```
This starts a local web-server with hot-reloading support.<br>
You can access the page by opening the link it prints out.

To make a production-ready build, run this instead:
```sh
yarn build
```
This will create a minified, static website inside the `/dist` directory.<br>
If you want to host this page, make sure to set the base-path correctly (set in the `package.json`).<br>
By default, it's set to `/rspl` since that's what the GitLab page of this repo needs.<br>

### CLI
To build the CLI instead, run:
```sh
yarn build:cli
```
A single file called `cli.mjs` inside the `/dist` directory will be build.<br>
This file is completely self-contained, meaning it no longer needs any external packages and can be moved into any other directory.

> **Note**<br> 
> If you switch between building the Web-App and CLI, make a clean build first. <br>
> To clean any build files, run: `yarn clean`

#### Usage
To transpile files using the CLI, start it with NodeJS:
```sh
node cli.mjs inputFile.rspl
```
A new file in the same location as the input will be created, with the extension `.S`.<br>
This can then be used as-is in a libdragon project.

### Language Definition
RSPL itself is defined in a grammar file, using the libraries [Nearly.js](https://nearley.js.org/) & [Moo](https://github.com/no-context/moo) for tokenization and lexing.<br/>
It can be found in: `src/lib/grammar.ne`<br/>
This is not directly used by the CLI/WebAPP, but rather nearly.js can be used to produce a JS file from it which acts as the lexer/parser.<br/>
To regenerate that file, run:
```sh
yarn build:lang
```
To efficiently make changes to the grammar file,<br/>
i recommend using the online tool nearly provides: https://omrelli.ug/nearley-playground/

> **Note**<br> 
> The produced JS file is checked into this repo.<br/>
> You only need to run this step when making changes to `src/lib/grammar.ne`.

## Tests
This project uses `jest` for automated testing.<br>
Tests are located in `src/tests` and mostly contain unit tests for syntax and operations, as well as a few complete example files.<br/>
To execute them, run:
```sh
yarn test
```
For an interactive mode, which watches for changes, run:
```
yarn test --watch
```

## Types
While this project is written in plain JS, it still makes use of type-hints.<br>
This is using a combination of [JSDoc](https://jsdoc.app/) and [TypeScript](https://www.typescriptlang.org/)-Definitions.<br>
Type-definition files are located in `src/lib/types`.<br>
Given an IDE that can understand these, it will provide auto-completion and type-checking.

> **Note**<br>
> There is no need to install TypeScript, IDEs will understand the type-hints without it.

## License
This software is licensed under "Apache License 2.0".<br>

Note that this license does NOT apply to any RSPL code written by users of this software, including the transpiled assembly output.<br>
I do NOT claim any copyright, nor do i enforce any restrictions on generated assembly code produced by this software.

© Max Bebök (HailToDodongo)