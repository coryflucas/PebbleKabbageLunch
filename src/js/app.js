Pebble.addEventListener('ready', function(e) {
  Pebble.timelineSubscribe('KabbageLunches', 
    function () { 
      console.log('Subscribed to KabbageLunches');
    }, 
    function (errorString) { 
      console.log('Error subscribing to topic: ' + errorString);
    }
  );
});