let debounceTimeout;

async function httpGetAsync(theUrl, callback) {
    try {
        const response = await fetch(theUrl);
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        const data = await response.text();
        callback(data);
    } catch (error) {
        console.error('Error:', error);
    }
}

const colorPicker = new iro.ColorPicker("#picker", {
    width: 280,
    borderWidth: 5,
    borderColor: "#000",
});

function updateColor(color) {
    const rgb = color.rgb;
    const url = `/rgb?red=${rgb.r}&green=${rgb.g}&blue=${rgb.b}`;
    httpGetAsync(url, console.log);
}

colorPicker.on('color:change', (color) => {
    clearTimeout(debounceTimeout);
    debounceTimeout = setTimeout(() => {
        updateColor(color);
    }, 10);
});

colorPicker.on('input:end', (color) => {
    const rgb = color.rgb;
    document.getElementById("rgb_r").value = rgb.r;
    document.getElementById("rgb_g").value = rgb.g;
    document.getElementById("rgb_b").value = rgb.b;
    updateColor(color);
});

function updateForm(data) {
    const fields = [
        'ssid',
        'pass',
        'colon',
        'ntp',
        'time',
        'color'
    ];
    fields.forEach(field => {
        console.log(data);
        if (field === 'time') {
            const city = data[field].city || '';
            document.getElementById('time_fmt').value = data[field].time_fmt || '';
            const selectElement = document.getElementById('timezone');
            const options = selectElement.options;

            for (let i = 0; i < options.length; i++) {
                if (options[i].text === city) {
                    options[i].selected = true;
                    break;
                }
            }
        } else if (field == 'color') {
            colorPicker.color.rgb = data['color'];
        } else {
            document.getElementById(field).value = data[field] || '';
        }
    });
}

async function fetchData() {
    try {
        const response = await fetch('/data');
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        const jsonData = await response.json();
        updateForm(jsonData);
    } catch (error) {
        console.error('Error fetching data:', error);
    }
}

async function updateValues(event) {
    event.preventDefault();

    const formData = new FormData(event.target);
    const data = Object.fromEntries(formData.entries());

    const selectElement = document.getElementById('timezone');
    const selectedOption = selectElement.options[selectElement.selectedIndex];
    const city = selectedOption.textContent;
    const timezone = selectedOption.value;

    const newData = {
        ssid: data.ssid,
        pass: data.pass,
        time_fmt: data.time_fmt,
        ntp: data.ntp,
        colon: data.colon,
        color: {
            r: data.rgb_r,
            g: data.rgb_g,
            b: data.rgb_b
        },
        time: {
            city: city,
            timezone: timezone,
            time_fmt: data.time_fmt
        }
    };

    console.log(newData);

    try {
        const response = await fetch('/update', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(newData)
        });
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
    } catch (error) {
        console.error('Error updating values:', error);
    }
}

document.addEventListener('DOMContentLoaded', () => {
    fetchData();
    document.getElementById('update-form').addEventListener('submit', updateValues);
});

async function rebootClock() {
    // Disable the button to prevent multiple clicks
    document.getElementById('rebootButton').disabled = true;

    try {
        // Perform the reboot action
        const response = await fetch('/reboot', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ action: 'reboot' })
        });
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        console.log('Clock rebooted successfully');
    } catch (error) {
        console.error('Error rebooting clock:', error);
    } finally {
        // Re-enable the button after the action is complete (success or error)
        document.getElementById('rebootButton').disabled = false;
    }
}

function resizePicker() {
    colorPicker.resize(window.innerWidth < 500 ? 280 : 400);
}

window.addEventListener("resize", resizePicker);
resizePicker();
