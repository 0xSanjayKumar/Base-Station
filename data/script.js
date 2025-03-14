let esp32IpAddress = "http://192.168.4.1"; // Default ESP32 AP Mode IP

async function sendCommand(command, params = {}) {
    const url = new URL(`${esp32IpAddress}/control`);
    url.searchParams.append('command', command);
    for (const key in params) {
        url.searchParams.append(key, params[key]);
    }

    fetch(url)
        .then(response => response.text())
        .then(data => console.log(`Command Response: ${data}`))
        .catch(error => console.error('Error:', error));
}

document.addEventListener('DOMContentLoaded', function() {
    // Add click effect to all buttons
    const buttons = document.querySelectorAll('button');
    buttons.forEach(button => {
        button.addEventListener('click', function() {
            this.style.transform = 'scale(0.95)';
            setTimeout(() => {
                this.style.transform = 'scale(1)';
            }, 100);
        });
    });

    // Demo: Show connection status
    const connectionIndicator = document.getElementById('connectionIndicator');
    connectionIndicator.classList.add('active');

    // Create sample interactivity
    document.getElementById('updateButton').addEventListener('click', function() {
        const latitude = document.getElementById('latitude').value;
        const longitude = document.getElementById('longitude').value;
        const altitude = document.getElementById('altitude').value;
        const accuracy = document.getElementById('accuracy').value;
        const time = document.getElementById('time').value;

        sendCommand('Update', { latitude, longitude, altitude, accuracy, time });

        document.getElementById('dataIndicator').classList.add('active');
        setTimeout(() => {
            document.getElementById('gpsIndicator').classList.add('active');
        }, 1000);
    });

    document.getElementById('stopButton').addEventListener('click', function() {
        sendCommand('Stop');
        document.getElementById('dataIndicator').classList.remove('active');
        document.getElementById('gpsIndicator').classList.remove('active');
    });

    document.getElementById('autoSurveyButton').addEventListener('click', function() {
        sendCommand('Auto-Survey');
    });

    document.getElementById('autoFixButton').addEventListener('click', function() {
        sendCommand('Auto-Fix');
    });

    document.getElementById('resetButton').addEventListener('click', function() {
        sendCommand('Reset');
    });

    document.getElementById('portButton').addEventListener('click', function() {
        sendCommand('Port');
    });

    document.getElementById('msgEnableButton').addEventListener('click', function() {
        sendCommand('MSG-Enable');
    });

    document.getElementById('surveyStatButton').addEventListener('click', function() {
        sendCommand('Survey_Stat');
    });

    document.getElementById('surveyingButton').addEventListener('click', function() {
        const accuracy = document.getElementById('accuracy').value || 2.0;
        const time = document.getElementById('time').value || 200;
        sendCommand('Surveying', { accuracy, time });
    });

    document.getElementById('broadcastButton').addEventListener('click', function() {
        sendCommand('Broadcast');
    });

    document.getElementById('statusButton').addEventListener('click', function() {
        sendCommand('Status');
    });

    // Add event listeners for Manual Mode Fix buttons
    document.getElementById('resetButtonFix').addEventListener('click', function() {
        sendCommand('Reset');
    });

    document.getElementById('portButtonFix').addEventListener('click', function() {
        sendCommand('Port');
    });

    document.getElementById('msgEnableButtonFix').addEventListener('click', function() {
        sendCommand('MSG-Enable');
    });

    document.getElementById('fixModeButtonFix').addEventListener('click', function() {
        sendCommand('Fix-Mode');
    });

    document.getElementById('broadcastButtonFix').addEventListener('click', function() {
        sendCommand('Broadcast');
    });

    document.getElementById('statusButtonFix').addEventListener('click', function() {
        sendCommand('Status');
    });

    // Get the help toggle button and content
    const helpToggle = document.getElementById('helpToggle');
    const helpContent = document.getElementById('helpContent');

    // Add click event listener to the toggle button
    helpToggle.addEventListener('click', function() {
        // Toggle the 'show' class on the content
        helpContent.classList.toggle('show');

        // Change the arrow direction
        if (helpContent.classList.contains('show')) {
            helpToggle.textContent = '▲'; // Up arrow when content is visible
        } else {
            helpToggle.textContent = '▼'; // Down arrow when content is hidden
        }
    });
});