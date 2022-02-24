import os
from fastapi import FastAPI
from pydantic import BaseModel

from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

token = os.getenv("TOKEN_INFLUX")
org = "test"
bucket = "weather"

client = InfluxDBClient(url="http://localhost:8086", token=token, org=org)
write_api = client.write_api(write_options=SYNCHRONOUS)


app = FastAPI()


class Data(BaseModel):
    tempc: float
    humidity: float
    s_state: int


@app.get("/items")
def items():
    return {"items": [1, 2, 3, 4]}


@app.post("/add_data")
def create_data(data: Data):
    print(f"Posting data {data}")

    seq = [
        f"temp_C,host=host1 temp={data.tempc}",
        f"hum,host=host1 humidity={data.humidity}",
        f"sensor,host=host1 sensor_state={data.s_state}"
    ]
    try:
        write_api.write(bucket, org, seq)
    except Exception:
        print("ERROR de conexión")
    else:
        return data
