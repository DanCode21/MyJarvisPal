# weather - Gets weather information from OpenWeather API.

import requests
from System.config import WEATHER_API_KEY

def get_weather(city):
    """Return a weather report for a given city."""
    try:
        url = f"https://api.openweathermap.org/data/2.5/weather?q={city}&appid={WEATHER_API_KEY}&units=metric"
        data = requests.get(url).json()

        if data.get("cod") != 200:
            return f"I cannot find the weather for {city}"

        temp = data["main"]["temp"]
        desc = data["weather"][0]["description"]

        return f"The temperature in {city} is {temp} degrees with {desc}."
    except:
        return "Weather service error."
