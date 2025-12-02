# system_time.py
# Provides the current time for the voice assistant for cities 

import datetime # for current time 
import pytz #lets us handle differnt timezones 

#maps the known intent cities to time zone
CITY_TIMEZONES = {
    "burnaby": "America/Vancouver",
    "vancouver": "America/Vancouver",
    "surrey": "America/Vancouver",
    "richmond": "America/Vancouver",
    "new york city": "America/New_York",

    "rome": "Europe/Rome",
    "london": "Europe/London",
    "susa": "Asia/Tehran", 
    "denver": "America/Denver",
    "barcelona": "Europe/Madrid",
    "beijing": "Asia/Shanghai",
    "osaka": "Asia/Tokyo",
    "moscow": "Europe/Moscow",
}

def get_time(city=None):
    # we say a city then:
    if city is not None and city != "":
        city_lower = city.lower() #converts all string to lowercase

        # if city said is in our defintions then:
        if city_lower in CITY_TIMEZONES:
            try:
                tz = pytz.timezone(CITY_TIMEZONES[city_lower]) # we look up the timezone
                current_time = datetime.datetime.now(tz) # get current time in city
                return current_time.strftime("%I:%M %p") # display time in 00:00 am/pm format 
            except:
                return f"Error getting time for {city}"

        else:
            return f"Sorry sir I don't recognize the city {city}"

    # Default: Beagle's local time
    return datetime.datetime.now().strftime("%I:%M %p")
