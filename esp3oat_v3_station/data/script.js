setInterval(function DSR_BAR() {
    var meter3 = document.getElementById('wire2');
    var val3 = document.getElementById('wire');
    var Length3 = 0;
    Length3 = val3.innerHTML;
    meter3.value = Length3;
}, 2500);

setInterval(function DSR_BAR() {
    var meter = document.getElementById('dsr2');
    var val = document.getElementById('dsr');
    var Length = 0;
    Length = val.innerHTML;
    meter.value = Length;
}, 2500);

setInterval(function SGD() {
    var meter2 = document.getElementById('sgd2');
    var val2 = document.getElementById('sgd');
    var Length2 = 0;
    Length = val2.innerHTML;
    meter2.value = Length2;
}, 2500);

setInterval(function BATB() {
    var meter4 = document.getElementById('bat_boat2');
    var val4 = document.getElementById('bat_boat');
    var Length4 = 0;
    Length4 = val4.innerHTML;
    meter4.value = Length4;
}, 2500);

setInterval(function BATC() {
    var meter5 = document.getElementById('bat_cont2');
    var val5 = document.getElementById('bat_cont');
    var Length5 = 0;
    Length5 = val5.innerHTML;
    meter5.value = Length5;
}, 2500);

function openNav() {
  document.getElementById("myNav").style.width = "100%";
}

function closeNav() {
  document.getElementById("myNav").style.width = "0%";
}

// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);
window.addEventListener('load', getReadings2);

// Create Temperature Chart
var chartT = new Highcharts.Chart({
    chart: {
        renderTo: 'chart-temperature'
    },
    series: [
        {
            name: 'Temperature air (degC)',
            type: 'line',
            color: '#101D42',
            marker: {
                symbol: 'circle',
                radius: 3,
                fillColor: '#101D42',
            }
        },
        {
            name: 'Temperature eau (degC)',
            type: 'line',
            color: '#00A6A6',
            marker: {
                symbol: 'square',
                radius: 3,
                fillColor: '#00A6A6',
            }
        },
        {
            name: 'Humidite (%)',
            type: 'line',
            color: '#8B2635',
            marker: {
                symbol: 'triangle',
                radius: 3,
                fillColor: '#8B2635',
            }
        },
    ],
    title: {
        text: undefined
    },
    xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
        title: {
            text: 'Value'
        }
    },
    credits: {
        enabled: false
    }
});

// Create Course Chart
var chartC = new Highcharts.Chart({
    chart: {
        renderTo: 'chart-course'
    },
    series: [
        {
            name: 'Course',
            type: 'scatter',
            color: '#101D42',
            marker: {
                symbol: 'circle',
                radius: 3,
                fillColor: '#101D42',
            },
            lineWidth: 1
        },
    ],
    title: {
        text: undefined
    },
    xAxis: {
        title: {
            text: 'Latitude'
        }
    },
    yAxis: {
        title: {
            text: 'Longitude'
        }
    },
    credits: {
        enabled: false
    }
});

// Create Profondeur Chart
var chartP = new Highcharts.Chart({
    chart: {
        renderTo: 'chart-profondeur'
    },
    series: [
        {
            name: 'Course',
            type: 'line',
            color: '#101D42',
            marker: {
                symbol: 'circle',
                radius: 3,
                fillColor: '#101D42',
            }
        },
    ],
    title: {
        text: undefined
    },
    xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
        title: {
            text: 'Profondeur (cm)'
        }
    },
    credits: {
        enabled: false
    }
});

// Create Vitesse Chart
var chartV = new Highcharts.Chart({
    chart: {
        renderTo: 'chart-vitesse'
    },
    series: [
        {
            name: 'Course',
            type: 'line',
            color: '#101D42',
            marker: {
                symbol: 'circle',
                radius: 3,
                fillColor: '#101D42',
            }
        },
    ],
    title: {
        text: undefined
    },
    xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
        title: {
            text: 'Vitesse (km/h)'
        }
    },
    credits: {
        enabled: false
    }
});

//Plot charts
function plotTemperature(jsonValue) {

    var keys = Object.keys(jsonValue);
    console.log(keys);
    console.log(keys.length);

    for (var i = 0; i < keys.length; i++) {
        var t = (new Date()).getTime() + 7200000;
        console.log(t);
        const key = keys[i];

        if (i <= 2) {
            var y = Number(jsonValue[key]);
            if (chartT.series[i].data.length > 40) {
                chartT.series[i].addPoint([t, y], true, true, true);
            } else {
                chartT.series[i].addPoint([t, y], true, false, true);
            }
        }
        else if (i == 3)
            var lt = Number(jsonValue[key]);
        else if (i == 4) {
            var ln = Number(jsonValue[key]);
            chartC.series[0].addPoint([lt, ln], true, false, true);
        }
        else if (i == 5) {
            var prof = Number(jsonValue[key]);
            if (chartP.series[0].data.length > 40) {
                chartP.series[0].addPoint([t, prof], true, true, true);
            } else {
                chartP.series[0].addPoint([t, prof], true, false, true);
            }
        }
        else if (i == 6) {
            var prof = Number(jsonValue[key]);
            if (chartV.series[0].data.length > 40) {
                chartV.series[0].addPoint([t, prof], true, true, true);
            } else {
                chartV.series[0].addPoint([t, prof], true, false, true);
            }
        }
    }
}


// Function to get current readings on the webpage when it loads for the first time
function getReadings2() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            console.log(myObj);
            plotTemperature(myObj);
        }
    };
    xhr.open("GET", "/readings2", true);
    xhr.send();
}





// Create Temperature Gauge
var gaugeTemp = new LinearGauge({
  renderTo: 'gauge-temperature',
  width: 80,
  height: 200,
  units: "Temperature C",
  minValue: 0,
  startAngle: 90,
  ticksAngle: 180,
  maxValue: 40,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueDec: 2,
  valueInt: 2,
  majorTicks: [
      "0",
      "5",
      "10",
      "15",
      "20",
      "25",
      "30",
      "35",
      "40"
  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 30,
          "to": 40,
          "color": "rgba(200, 50, 50, .75)"
      }
  ],
  colorPlate: "#fff",
  colorBarProgress: "#CC2936",
  colorBarProgressEnd: "#049faa",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  barWidth: 10,
}).draw();
  
// Create Humidity Gauge
var gaugeHum = new RadialGauge({
  renderTo: 'gauge-humidity',
  width: 180,
  height: 180,
  units: "Humidity (%)",
  minValue: 0,
  maxValue: 100,
  colorValueBoxRect: "#049faa",
  colorValueBoxRectEnd: "#049faa",
  colorValueBoxBackground: "#f1fbfc",
  valueInt: 2,
  majorTicks: [
      "0",
      "20",
      "40",
      "60",
      "80",
      "100"

  ],
  minorTicks: 4,
  strokeTicks: true,
  highlights: [
      {
          "from": 80,
          "to": 100,
          "color": "#03C0C1"
      }
  ],
  colorPlate: "#fff",
  borderShadowWidth: 0,
  borders: false,
  needleType: "line",
  colorNeedle: "#007F80",
  colorNeedleEnd: "#007F80",
  needleWidth: 2,
  needleCircleSize: 3,
  colorNeedleCircleOuter: "#007F80",
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear"
}).draw();

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var temp = myObj.temperature;
      var hum = myObj.humidity;
      gaugeTemp.value = temp;
      gaugeHum.value = hum;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    gaugeTemp.value = myObj.temperature;
    gaugeHum.value = myObj.humidity;
  }, false);

    source.addEventListener('new_readings2', function (e) {
        console.log("new_readings2", e.data);
        var myObj = JSON.parse(e.data);
        console.log(myObj);
        plotTemperature(myObj);
    }, false);
}

function onButton() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "on", true);
    xhttp.send();
}

function offButton() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "off", true);
    xhttp.send();
}

function captureButton() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "capture", true);
    xhttp.send();
}

function captureButtonWire() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "capturewire", true);
    xhttp.send();
}


function captureButtonSub() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "capturesub", true);
    xhttp.send();
}


function captureButtonWS() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "watersampling", true);
    xhttp.send();
}

function myFunction() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "mode", true);
    xhttp.send();
}

function myFunction5() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "modeAuto", true);
    xhttp.send();
}

function myFunction2() {
  var x = document.getElementById("myDIV2");
  if (x.innerHTML === "ThingSpeak ON") {
    x.innerHTML = "ThingSpeak OFF";
  } else {
    x.innerHTML = "ThingSpeak ON";
  }
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "thingSpeak", true);
    xhttp.send();
}


function myFunction3() {
  var x = document.getElementById("myDIV3");
  if (x.innerHTML === "SD Card ON") {
    x.innerHTML = "SD CARD ON";
  } else {
    x.innerHTML = "SD CARD ON";
  }
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "sd", true);
    xhttp.send();
}


function Coord() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "coord", true);
    xhttp.send();
}

function AutoStart() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "autostart", true);
    xhttp.send();
}

function AutoStop() {
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", "autostop", true);
    xhttp.send();
}



setInterval(function getData()
{
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function()
    {
        if(this.readyState == 4 && this.status == 200)
        {
            document.getElementById("dsr").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "distanceSR04", true);
    xhttp.send();
}, 2500);

setInterval(function getData2()
{
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function()
    {
        if(this.readyState == 4 && this.status == 200)
        {
            document.getElementById("wire").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "wire", true);
    xhttp.send();
}, 2500);

setInterval(function getData21()
{
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function()
    {
        if(this.readyState == 4 && this.status == 200)
        {
            document.getElementById("iwire").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "wireinit", true);
    xhttp.send();
}, 3500);

setInterval(function getData3() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("sgd").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "seagroundDistance", true);
    xhttp.send();
}, 5000);


setInterval(function getBatteryBoat() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("bat_boat").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "BatBoat", true);
    xhttp.send();
}, 2500);

setInterval(function getBatteryContr() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("bat_cont").innerHTML = this.responseText;
        }
    };

    xhttp.open("GET", "BatContr", true);
    xhttp.send();
}, 2500);






function NewParcours() {
    document.getElementById('lat1').value = "";
    document.getElementById('lon1').value = "";
    document.getElementById('lat2').value = "";
    document.getElementById('lon2').value = "";
    document.getElementById('lat3').value = "";
    document.getElementById('lon3').value = "";
    document.getElementById('lat4').value = "";
    document.getElementById('lon4').value = "";
    document.getElementById('lat5').value = "";
    document.getElementById('lon5').value = "";
    document.getElementById('lat6').value = "";
    document.getElementById('lon6').value = "";
    document.getElementById('lat7').value = "";
    document.getElementById('lon7').value = "";
    document.getElementById('lat8').value = "";
    document.getElementById('lon8').value = "";
    document.getElementById('lat9').value = "";
    document.getElementById('lon9').value = "";
    document.getElementById('lat10').value = "";
    document.getElementById('lon10').value = "";
}



function SendParcours() {
    $.post("SendCoord",
        {
            lat1: document.getElementById('lat1').value,
            lon1: document.getElementById('lon1').value,
            lat2: document.getElementById('lat2').value,
            lon2: document.getElementById('lon2').value,
            lat3: document.getElementById('lat3').value,
            lon3: document.getElementById('lon3').value,
            lat4: document.getElementById('lat4').value,
            lon4: document.getElementById('lon4').value,
            lat5: document.getElementById('lat5').value,
            lon5: document.getElementById('lon5').value,
            lat6: document.getElementById('lat6').value,
            lon6: document.getElementById('lon6').value,
            lat7: document.getElementById('lat7').value,
            lon7: document.getElementById('lon7').value,
            lat8: document.getElementById('lat8').value,
            lon8: document.getElementById('lon8').value,
            lat9: document.getElementById('lat9').value,
            lon9: document.getElementById('lon9').value,
            lat10: document.getElementById('lat10').value,
            lon10: document.getElementById('lon10').value
        });
}


function SavePID() {
    $.post("SendPID",
        {
            Kp: document.getElementById('Kp').value,
            Ki: document.getElementById('Ki').value,
            Kd: document.getElementById('Kd').value
        });
}


function logoutButton() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/logout", true);
    xhr.send();
    setTimeout(function () { window.open("/logged-out", "_self"); }, 1000);
}


