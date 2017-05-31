myApp.filter("formatDate", function() {
  return function(value) {
    value = new Date(value * 1000);
    var hrs, minutes;
    hrs = value.getHours();
    minutes = value.getMinutes();
    hrs = hrs.length > 2 ? "0" + hrs.toString() : hrs.toString();
    minutes = minutes.length > 2 ? "0" + minutes.toString() : minutes.toString();
    
    return hrs + ":" + minutes; 
  };
});

myApp.filter("stringifyJSON", function() {
	return function(value) {
		return JSON.stringify(value,null,2);
	};

});
