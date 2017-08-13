var lunchApiBaseUrl = 'http://lunch.kabbage.com/api/v2'

var sendMenu = function(menu) {
    Pebble.sendAppMessage({
        'LUNCH_MENU': menu
    });
};

var fetchLunch = function(date, menuCallback) {
    console.log('Requesting lunch for ' + date);
    var request = new XMLHttpRequest();
    request.onload = function() {
        if(this.status == 200) {
            var lunch = JSON.parse(this.responseText);
            if(lunch.menu) {
                console.log('Found menu: ' + lunch.menu);
                menuCallback(lunch.menu);
            }
            else {
                console.log('Menu not found in response');
                menuCallback('Unknown');
            }
        }
        else {
            console.log('Lunch API failed to locate lunch');
            menuCallback('Unknown');
        }
    };

    request.open('GET', lunchApiBaseUrl + '/lunches/' + date);
    request.send()
}

Pebble.addEventListener('ready', function (e) {
    console.log('PebbleKit JS ready.');

    Pebble.timelineSubscribe('KabbageLunch',
      function () {
        console.log('Subscribed to KabbageLunch');
      },
      function (errorString) {
        console.log('Error subscribing to topic: ' + errorString);
      }
    );

    Pebble.addEventListener('appmessage', function(e) {
        var dict = e.payload;

        if(dict['LUNCH_DATE']) {
            var date = dict['LUNCH_DATE'];
            fetchLunch(date, sendMenu);
        }
    });

    Pebble.sendAppMessage({'JS_READY': 1});
});

