## Getting Started

Install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html).

Compile this demo `snow.cpp` into WebAssembly:

```
emcc snow.cpp -o snow.html
```

This will output a few files in the current directory:
- `snow.html` and `snow.js` - A single page web app.
- `snow.wasm` - A binary WebAssembly file.

Start a local webserver:
```
npx serve
```

Open the generated single page app: http://localhost:5000/snow

The app will make an XHR request to fetch `snow.wasm`, and run it.

Try clicking different points on the canvas to create an interesting image. Here is an example:

![snow](https://user-images.githubusercontent.com/346214/116622461-a5386300-a93c-11eb-956b-eeed7e652e42.gif)
