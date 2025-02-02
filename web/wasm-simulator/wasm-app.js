class EmscriptenApp extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    this.render();
    this.setupModule();
  }

  render() {
    this.innerHTML = `
			<style>
				.emscripten {
					padding-right: 0;
					margin-left: auto;
					margin-right: auto;
					display: block;
				}

				div.emscripten {
					text-align: center;
				}

				div.emscripten_border {
					border: 1px solid black;
				}

				canvas.emscripten {
					border: 0px none;
					background-color: black;
				}

				#output {
					width: 100%;
					height: 200px;
					margin: 10px 0;
					background-color: #303446;
					color: #c6d0f5;
					font-family: 'Lucida Console', Monaco, monospace;
					outline: none;
					resize: none;
					display: block;
				}
			</style>

			<div class="emscripten">
				<progress value="0" max="100" id="progress" hidden></progress>
			</div>

			<div class="emscripten_border">
				<canvas class="emscripten" id="canvas" oncontextmenu="event.preventDefault()" tabindex="-1"></canvas>
			</div>
			<textarea id="output" rows="8"></textarea>
		`;

    this.loadEmscriptenScript();
  }

  setupModule() {
    setTimeout(() => {
      const outputElement = this.querySelector("#output");
      const canvasElement = this.querySelector("#canvas");

      window.Module = {
        print: (...args) => {
          const text = args.join(" ");
          console.log(text);
          if (outputElement) {
            outputElement.value += text + "\n";
            outputElement.scrollTop = outputElement.scrollHeight;
          }
        },
        canvas: canvasElement,
        totalDependencies: 0,
        monitorRunDependencies: (left) => {
          this.totalDependencies = Math.max(this.totalDependencies, left);
        },
      };

      canvasElement.addEventListener("webglcontextlost", (e) => {
        alert("WebGL context lost. You will need to reload the page.");
        e.preventDefault();
      });

      window.onerror = (event) => {
        console.error(event);
      };
    }, 0);
  }

  loadEmscriptenScript() {
    const scriptSrc = this.getAttribute("script") || "badge2024_c.js";

    const script = document.createElement("script");
    script.src = scriptSrc;
    script.async = true;
    script.onload = () => console.log(`${scriptSrc} loaded successfully`);
    script.onerror = () => console.error(`Failed to load ${scriptSrc}`);

    document.body.appendChild(script);
  }
}

customElements.define("emscripten-app", EmscriptenApp);

