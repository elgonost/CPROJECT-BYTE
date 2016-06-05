var buffer_size = 900;
// Changes with sampling rate
// 10 Hz : ms = 100 | 25 Hz : ms = 40 | 50 Hz : ms = 20 | 100 Hz : ms = 10
var ms = 40; 
var time_samples = buffer_size / 25;
var xarray = [];
var yarray = [];
var zarray = [];
var timearray = [];
var startarray = [];
var endarray = [];
var activityarray = [];
var sendData = {};
var username = localStorage.getItem("username");
var numberOfMessages = 0;
var request = new XMLHttpRequest();


console.log("* * *Mobile phone says hello!!!* * *");

function convertBytesToAccel(msB,lsB){
  var temp = msB | 127;
  if (temp!=255)
    return (msB*100 + lsB);
  else{
    msB = msB & 127;
    return -(msB*100 + lsB);
  }  
}

function convertBytesToTime(byte6,byte5,byte4,byte3,byte2,byte1,byte0){
  return (byte6*1000000000000 + byte5*10000000000 + byte4*100000000 + byte3*1000000 + byte2*10000 + byte1*100 + byte0);
}

// Function to send a message to the Pebble using AppMessage API
// We are currently only sending a message using the "status" appKey defined in appinfo.json/Settings
function sendMessage() {
	Pebble.sendAppMessage({"status": 1}, messageSuccessHandler, messageFailureHandler);
}

// Called when the message send attempt succeeds
function messageSuccessHandler() {
  console.log("Message send succeeded.");  
}

// Called when the message send attempt fails
function messageFailureHandler() {
  console.log("Message send failed.");
  sendMessage();
}



Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://e088a03dc67dacfb01333eea79f5192d2b79960c.googledrive.com/host/0By0ivjD6euU1ZDlBLWZ4YmFMT2s/form.html';
  console.log('Showing configuration page: '+ url);
  Pebble.openURL(url);
});


Pebble.addEventListener('webviewclosed', function(e) {
  // Decode the user's preferences
  var configData = JSON.parse(decodeURIComponent(e.response));
  username = configData.userName;
  localStorage.setItem("username",username);
});





// Called when JS is ready
Pebble.addEventListener("ready", function(e) {
  console.log("JS is ready!");
  sendMessage();
});
						


// Called when incoming message from the Pebble is received
// We are currently only checking the "message" appKey defined in appinfo.json/Settings
Pebble.addEventListener("appmessage", function(e) {
  
    var start_temp;
    var end_temp;
    if(e.payload.timevalue !== undefined){
      
      numberOfMessages++;
      for(var i=0;i<buffer_size;i++){
        xarray.push(convertBytesToAccel(e.payload.xvalue[2*i],e.payload.xvalue[2*i+1]));
        yarray.push(convertBytesToAccel(e.payload.yvalue[2*i],e.payload.yvalue[2*i+1]));
        zarray.push(convertBytesToAccel(e.payload.zvalue[2*i],e.payload.zvalue[2*i+1]));
      }
      for(i=0;i<time_samples;i++){
        var temp = convertBytesToTime(e.payload.timevalue[7*i],e.payload.timevalue[7*i+1],e.payload.timevalue[7*i+2],
                                  e.payload.timevalue[7*i+3],e.payload.timevalue[7*i+4],e.payload.timevalue[7*i+5],
                                  e.payload.timevalue[7*i+6]);
    
        for(var j=0;j<25;j++){
          timearray.push(temp);
          temp += ms;
        }  
      }
      
      if(numberOfMessages==1){
        sendData = {"timestamp":timearray,"user":username,"xvalue":xarray,"yvalue":yarray,"zvalue":zarray,
                  "starttime":timearray[0],"endtime":timearray[timearray.length-1]};
        
        request.open('POST','http://83.212.115.163:8080/pebble');
        request.setRequestHeader('Content-Type', 'application/json; charset=utf-8');   
        request.send(JSON.stringify(sendData));
      
        xarray.length = 0;
        yarray.length = 0;
        zarray.length = 0;
        timearray.length = 0;
        numberOfMessages = 0;
        
      }
      
    }
    
    if(e.payload.actTimeValueStart !== undefined){
      console.log("e.payload.actTimeValueStart NOT undefined");
      console.log('**PHONE**: actTimeValueStart = '+ JSON.stringify(e.payload.actTimeValueStart));
      console.log('**PHONE**: actTimeValueEnd = '+ JSON.stringify(e.payload.actTimeValueEnd));
      console.log('**PHONE**: activity code = '+ JSON.stringify(e.payload.activity));
      console.log('**PHONE**: number of activities sent = '+ JSON.stringify(e.payload.activityNumber));
      
      for(var k=0;k<e.payload.activityNumber;k++){
            start_temp = convertBytesToTime(e.payload.actTimeValueStart[7*k],e.payload.actTimeValueStart[7*k+1],e.payload.actTimeValueStart[7*k+2],
                                            e.payload.actTimeValueStart[7*k+3],e.payload.actTimeValueStart[7*k+4],e.payload.actTimeValueStart[7*k+5],
                                            e.payload.actTimeValueStart[7*k+6]);
    
            end_temp = convertBytesToTime(e.payload.actTimeValueEnd[7*k],e.payload.actTimeValueEnd[7*k+1],e.payload.actTimeValueEnd[7*k+2],
                                          e.payload.actTimeValueEnd[7*k+3],e.payload.actTimeValueEnd[7*k+4],e.payload.actTimeValueEnd[7*k+5],
                                          e.payload.actTimeValueEnd[7*k+6]);
            startarray.push(start_temp);
            endarray.push(end_temp-5000);
            switch(e.payload.activity[k]){
              case 0:
                activityarray.push("Mild");
                break;
              case 1:
                activityarray.push("Moderate");
                break;
              case 2:
                activityarray.push("Intense");
                break;
              case 3:
                activityarray.push("Cycling");
                break;
              case 4:
                activityarray.push("Dressing");
                break;
              case 5:
                activityarray.push("Driving");
                break;
              case 6:
                activityarray.push("Fall");
                break;
              case 7:
                activityarray.push("Mobile");
                break;
              case 8:
                activityarray.push("Running");
                break;  
              case 9:
                activityarray.push("Sitting");
                break;
              case 10:
                activityarray.push("Sleeping early day");
                break;
              case 11:
                activityarray.push("Sleeping mid day");
                break;
              case 12:
                activityarray.push("Sleeping late day");
                break;
              case 13:
                activityarray.push("Standing");
                break;
              case 14:
                activityarray.push("Non Fall Spike");
                break;
              case 15:
                activityarray.push("Walking");
                break;
              case 16:
                activityarray.push("Washing in front of mirror");
                break;
              case 17:
                activityarray.push("Watching TV");
                break;
            }                
      }
      console.log("startarray is: "+ JSON.stringify(startarray));
      console.log("endarray is: "+ JSON.stringify(endarray));
      console.log("activityarray is: "+ JSON.stringify(activityarray));
      
    
      sendData = {"user":username,"activity":activityarray,"start":startarray,"end":endarray};
      request = new XMLHttpRequest();
      request.open('POST','http://83.212.115.163:8080/activity');
      request.setRequestHeader('Content-Type', 'application/json; charset=utf-8');   
      request.send(JSON.stringify(sendData));
      
      startarray.length = 0;
      endarray.length = 0;
      activityarray.length = 0;
    }
    
});