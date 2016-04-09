Pebble.addEventListener('ready', function(e) {
  Pebble.timelineSubscribe('KabbageLunch', 
    function () { 
      console.log('Subscribed to KabbageLunch');
    }, 
    function (errorString) { 
      console.log('Error subscribing to topic: ' + errorString);
    }
  );
});