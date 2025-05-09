// Complete project details: https://randomnerdtutorials.com/esp32-plot-readings-charts-multiple/

// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);
window.addEventListener('load', getHistory);
Highcharts.setOptions({
  time: {
    timezone: 'America/New_York' //Add your time zone offset here in minutes
  }
});

function getHistory(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          var history = JSON.parse(this.responseText);
          
          // Clear existing data
          chartT.series.forEach(function(series) {
              series.setData([]);
          });
          
          // Get current time and latest timestamp to calculate offset
          const now = Date.now();
          const latestReading = Math.max(...history.map(r => r.timestamp));
          const timeOffset = now - latestReading;
          
          // Plot historical data
          history.forEach(function(reading) {
              // Add timeOffset to convert from millis() to Unix epoch
              var x = reading.timestamp + timeOffset;
              chartT.series[0].addPoint([x, Number(reading.sensor1)], false);
              chartT.series[1].addPoint([x, Number(reading.sensor2)], false);
              chartT.series[2].addPoint([x, Number(reading.sensor3)], false);
              chartT.series[3].addPoint([x, Number(reading.sensor4)], false);
          });
          
          chartT.redraw();
          
          // Start getting real-time readings
          getReadings();
      }
  };
  xhr.open("GET", "/history", true);
  xhr.send();
}

// Create Temperature Chart
var chartT = new Highcharts.Chart({
	  time:{
	useUTC: false
  },
  chart:{
    renderTo:'chart-temperature'
  },
  series: [
    {
      name: 'Temp #1 (&deg;F)',
      type: 'line',
      color: '#101D42',
      marker: {
        symbol: 'circle',
        radius: 3,
        fillColor: '#101D42',
      }
    },
    {
      name: 'Temp #2 (&deg;F)',
      type: 'line',
      color: '#00A6A6',
      marker: {
        symbol: 'square',
        radius: 3,
        fillColor: '#00A6A6',
      }
    },
    {
      name: 'Set Temp (&deg;F)',
      type: 'line',
      color: '#8B2635',
      marker: {
        symbol: 'triangle',
        radius: 3,
        fillColor: '#8B2635',
      }
    },
	{
	  name: 'ETA (mins)',
      type: 'line',
      color: '#71B48D',
      marker: {
        symbol: 'triangle-down',
        radius: 3,
        fillColor: '#71B48D',
      }
    },
  ],
    plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: false }
    },
    series: { color: '#059e8a' }
  },
  title: {
    text: undefined
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: {
      text: 'Temperature Farenheit Degrees'
    }
  },
  credits: {
    enabled: false
  }
});


//Plot temperature in the temperature chart
function plotTemperature(jsonValue) {

  var keys = Object.keys(jsonValue);
  console.log(keys);
  console.log(keys.length);

  for (var i = 0; i < keys.length; i++){
    var x = (new Date()).getTime();
    console.log(x);
    const key = keys[i];
    var y = Number(jsonValue[key]);
    console.log(y);

    if(chartT.series[i].data.length > 240) {
      chartT.series[i].addPoint([x, y], true, true, true);
    } else {
      chartT.series[i].addPoint([x, y], true, false, true);
    }

  }
}

// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      plotTemperature(myObj);
	  document.getElementById("sensor1").innerHTML = myObj.sensor1;
	  document.getElementById("sensor2").innerHTML = myObj.sensor2;
	  document.getElementById("sensor3").innerHTML = myObj.sensor3;
	  document.getElementById("sensor4").innerHTML = myObj.sensor4;
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
    plotTemperature(myObj);
	document.getElementById("sensor1").innerHTML = myObj.sensor1;
	document.getElementById("sensor2").innerHTML = myObj.sensor2;
	document.getElementById("sensor3").innerHTML = myObj.sensor3;
	document.getElementById("sensor4").innerHTML = myObj.sensor4;
	
  }, false);
}
