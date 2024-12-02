import cv2
import numpy as np
import pyzbar.pyzbar as pyzbar
import urllib.request
import requests  # For sending data to ESP32

url = 'http://192.168.1.218/'
cv2.namedWindow("live transmission", cv2.WINDOW_AUTOSIZE)

while True:
    img_resp = urllib.request.urlopen(url + 'cam-hi.jpg')
    imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
    frame = cv2.imdecode(imgnp, -1)

    decodedObjects = pyzbar.decode(frame)
    for obj in decodedObjects:
        pres = obj.data.decode('utf-8')  # Decode the byte data to string
        print("Type:", obj.type)
        print("Data: ", pres)

        # Send data to ESP32
        try:
            response = requests.post(url + "receive-data", data=pres)
            print("ESP32 Response:", response.text)
        except Exception as e:
            print("Failed to send data to ESP32:", e)

        cv2.putText(frame, pres, (50, 50), cv2.FONT_HERSHEY_PLAIN, 2,
                    (255, 0, 0), 3)

    cv2.imshow("live transmission", frame)

    key = cv2.waitKey(1)
    if key == 27:
        breaks

cv2.destroyAllWindows()
