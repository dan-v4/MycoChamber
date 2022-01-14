function updateTimers(){
  var components = document.getElementsByClassName('TIMERTEXT');
  var url = "/timer?"+components[0].name+"="+components[0].value+"&"+
                      components[1].name+"="+components[1].value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var newValues = JSON.parse(this.responseText);
      document.getElementById("led-values").innerHTML = "ON: " + newValues.newON1 + " ms\nOFF: " + newValues.newOFF2 + " ms";
    }
  };
  xhr.open("POST", url);
  xhr.setRequestHeader('Content-TYPE', 'application/x-www-form-urlencoded');
  xhr.send();
}

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 4000) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 4000) ;

var humData = [];
var time = [];
var tempData = [];
var ctx = document.getElementById('HumidityChart').getContext('2d');
var ctx2 = document.getElementById('TemperatureChart').getContext('2d');
var humChart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: time,
    datasets: [{
      label: 'Tub Humidity (\u0025 RH)',
      data: humData ,
      backgroundColor: ['rgba(189, 240, 214, 1)'],
      borderColor: ['rgba(189, 240, 214, 1)'],
      borderWidth: 1
    }]
  },
  options:{
    scales:{
      x :{
        ticks:{
          color: 'rgba(189, 240, 214, 1)',
          maxTicksLimit: 50,
        }
      },
      y :{
        ticks:{
          color: 'rgba(189, 240, 214, 1)',
          maxTicksLimit: 50,
        }
      }
    },
    plugins:{
      legend:{
        labels:{
          color: 'rgba(189, 240, 214, 1)'
        }
      }
    }

  }
});

var tempChart = new Chart(ctx2, {
  type: 'line',
  data: {
    labels: time,
    datasets: [{
      label: 'Tub Temperature (\u2103)',
      data: tempData ,
      backgroundColor: ['#ffb347'],
      borderColor: ['#ffb347'],
      borderWidth: 1
    }]
  },
  options:{
    scales:{
      x :{
        ticks:{
          color: '#ffb347',
          maxTicksLimit: 50,
        }
      },
      y :{
        ticks:{
          color: '#ffb347',
          maxTicksLimit: 50,
        }
      }
    },
    plugins:{
      legend:{
        labels:{
          color: '#ffb347'
        }
      }
    }

  }
});
setInterval(function ( ) {
  var humidity = 0;
  var temp = 0;
  let now = new Date();
  let nLabel = now.getHours() + ':'+ now.getMinutes() + ':' + now.getSeconds();
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      humidity = parseFloat(this.responseText);
      humChart.data.labels.push(nLabel);
      humChart.data.datasets.forEach((dataset) => {
        dataset.data.push(humidity);
      });
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();

  var xhttp2 = new XMLHttpRequest();
  xhttp2.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      temp = parseFloat(this.responseText);
      tempChart.data.datasets.forEach((dataset) => {
        dataset.data.push(temp);
      });
    }
  };
  xhttp2.open("GET", "/temperature", true);
  xhttp2.send();

  if(nLabel.length > 400){
    humChart.data.labels.shift();
    humChart.data.datasets.forEach((dataset) => {
      dataset.data.shift();
    });
    tempChart.data.datasets.forEach((dataset) => {
      dataset.data.shift();
    });
  }

  humChart.update();
  tempChart.update();

}, 4000) ;
