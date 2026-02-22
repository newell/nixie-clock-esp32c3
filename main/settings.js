var colorPicker;
let debounceTimeout;

async function httpGetAsync(theUrl, callback) {
  try {
    const response = await fetch(theUrl);
    if (!response.ok) throw new Error("Network response was not ok");
    const data = await response.text();
    if (callback) callback(data);
  } catch (error) {
    console.error("Error:", error);
  }
}

function updateColor(color, force = false) {
  const mode = document.getElementById("led_mode").value;

  // If we aren't forcing it, block updates during spectrum mode
  if (!force && mode === "spectrum") return;

  const rgb = color.rgb;
  const url = `/rgb?red=${rgb.r}&green=${rgb.g}&blue=${rgb.b}`;

  // DEBUGGING
  // console.log("Sending Color Sync:", url);
  fetch(url)
    .then(() => console.log("Color sync successful"))
    .catch((err) => console.error("Color sync failed", err));
}

function toggleColorPicker(mode) {
  const pickerDiv = document.getElementById("picker");
  const rgbInputs = document.querySelectorAll("#rgb_r, #rgb_g, #rgb_b");

  if (mode === "spectrum") {
    pickerDiv.style.opacity = 0.3;
    pickerDiv.style.pointerEvents = "none";
    rgbInputs.forEach((input) => (input.disabled = true));
  } else {
    pickerDiv.style.opacity = 1;
    pickerDiv.style.pointerEvents = "auto";
    rgbInputs.forEach((input) => (input.disabled = false));
  }
}

async function fetchData() {
  try {
    const response = await fetch("/data");
    if (!response.ok) throw new Error("Network response was not ok");
    const jsonData = await response.json();
    updateForm(jsonData);
  } catch (error) {
    console.error("Error fetching data:", error);
  }
}

function updateForm(data) {
  const fields = ["ssid", "pass", "colon", "ntp", "time", "color", "led_mode"];

  fields.forEach((field) => {
    if (field === "time") {
      const city = data[field].city || "";
      document.getElementById("time_fmt").value = data[field].time_fmt || "";
      const selectElement = document.getElementById("timezone");
      for (let i = 0; i < selectElement.options.length; i++) {
        if (selectElement.options[i].text === city) {
          selectElement.options[i].selected = true;
          break;
        }
      }
    } else if (field == "color") {
      // Update the wheel and the numeric inputs
      colorPicker.color.rgb = data["color"];
      document.getElementById("rgb_r").value = data["color"].r;
      document.getElementById("rgb_g").value = data["color"].g;
      document.getElementById("rgb_b").value = data["color"].b;
    } else if (field === "led_mode") {
      const mode = data.led_mode || "static";
      document.getElementById("led_mode").value = mode;
      toggleColorPicker(mode);
    } else {
      const el = document.getElementById(field);
      if (el) el.value = data[field] || "";
    }
  });
}

async function sendModeUpdate(mode) {
  try {
    await fetch(`/led_mode?mode=${mode}`);
    console.log(`ESP32 RAM mode switched to: ${mode}`);
  } catch (error) {
    console.error("Error updating RAM mode:", error);
  }
}

async function updateValues(event) {
  event.preventDefault();

  const formData = new FormData(event.target);
  const data = Object.fromEntries(formData.entries());

  const selectElement = document.getElementById("timezone");
  const selectedOption = selectElement.options[selectElement.selectedIndex];

  // Ensure we have numbers, not empty strings, by pulling from the picker object
  const currentRGB = colorPicker.color.rgb;

  const newData = {
    ssid: data.ssid,
    pass: data.pass,
    time_fmt: data.time_fmt,
    ntp: data.ntp,
    colon: data.colon,
    led_mode: data.led_mode,
    color: {
      r: Math.round(currentRGB.r),
      g: Math.round(currentRGB.g),
      b: Math.round(currentRGB.b),
    },
    time: {
      city: selectedOption.textContent,
      timezone: selectedOption.value,
      time_fmt: data.time_fmt,
    },
  };

  console.log("Saving to ESP32:", newData);

  try {
    const response = await fetch("/update", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(newData),
    });

    if (!response.ok) throw new Error("Update failed");

    console.log("Settings successfully saved to Flash.");
  } catch (error) {
    console.error("Error updating values:", error);
  }
}

// Initialization starts here
document.addEventListener("DOMContentLoaded", () => {
  colorPicker = new iro.ColorPicker("#picker", {
    width: window.innerWidth < 500 ? 280 : 400,
    borderWidth: 5,
    borderColor: "#000",
  });
  fetchData();
  colorPicker.on("color:change", (color) => {
    clearTimeout(debounceTimeout);
    debounceTimeout = setTimeout(() => {
      updateColor(color);
    }, 10);
  });
  colorPicker.on("input:end", (color) => {
    const rgb = color.rgb;
    document.getElementById("rgb_r").value = rgb.r;
    document.getElementById("rgb_g").value = rgb.g;
    document.getElementById("rgb_b").value = rgb.b;
    updateColor(color);
  });

  document
    .getElementById("led_mode")
    .addEventListener("change", async function () {
      const selectedMode = this.value;
      toggleColorPicker(selectedMode);

      await sendModeUpdate(selectedMode);

      if (selectedMode === "static") {
        console.log("Forcing color sync after mode switch...");
        // We use 100ms to ensure the ESP32 HTTP server has closed the first socket
        setTimeout(() => {
          updateColor(colorPicker.color, true); // True = Force bypass
        }, 100);
      }
    });
  document
    .getElementById("update-form")
    .addEventListener("submit", updateValues);
});

function resizePicker() {
  if (colorPicker) {
    colorPicker.resize(window.innerWidth < 500 ? 280 : 400);
  }
}

window.addEventListener("resize", resizePicker);
