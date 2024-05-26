# Smart Relays API Backend

This API backend serves as the interface between the frontend interface for controlling smart relays and the underlying system managing the relay devices. Below are the details of the requests you need to make to interact with the backend, the corresponding responses, and possible error responses.

## Get All Relays

### Request

- **Endpoint**: `/api/all-relays`
- **Method**: GET

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
    "relays": [
      {
        "id": 0,
        "name": "Relay 1",
        "state": false
      },
      {
        "id": 1,
        "name": "Relay 1",
        "state": false
      }
    ]
  }
  ```

## Relay Control

### Request

- **Endpoint**: `/api/relay-control`
- **Method**: POST
- **Body**:
  ```json
  {
      "relayId": 1,
      "state": true // or false
  }
  ```

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Relay state updated successfully",
      "relayId": 1,
      "state": true // or false
  }
  ```

### Error Responses

- **Status**: 400 Bad Request
- **Body**:
  ```json
  {
      "error": "Missing or invalid parameters"
  }
  ```

- **Status**: 404 Not Found
- **Body**:
  ```json
  {
      "error": "Relay not found"
  }
  ```

## Current System Settings Retrieval

### Request

- **Endpoint**: `/api/settings`
- **Method**: GET

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "systemName": "Smart Relays",
      "wifiName": "YourWiFiName",
      "systemTime": "11:32:45",
      "systemDate": "2024-07-23",
      "syncTime": true,
      "relays": [
        {
            "id": 1,
            "name": "New Relay 1",
            "state": false
        },
        {
            "id": 2,
            "name": "New Relay 2",
            "state": true
        }
      ]
  }
  ```

## General Settings Update

### Request

- **Endpoint**: `/api/settings`
- **Method**: POST
- **Body**:
  ```json
  {
      "systemName": "New System Name",
      "wifiName": "New WiFi Name",
      "wifiPassword": "New WiFi Password",
      "systemTime": "14:30:00",
      "systemDate": "2024-05-13",
      "syncTime": true, // or false
      "relays": [
        {
            "id": 1,
            "name": "New Relay 1"
        },
        {
            "id": 2,
            "name": "New Relay 2"
        }
    ]
  }
  ```

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Settings updated successfully"
  }
  ```

### Error Responses

- **Status**: 400 Bad Request
- **Body**:
  ```json
  {
      "error": "Invalid data provided"
  }
  ```

## Get All Alarms for a Specific Relay

### Request

- **Endpoint**: `/api/relay-alarms/:relayId`
- **Method**: GET

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  [
      {
          "id": 1,
          "state": true,
          "time": "06:00:00",
          "days": ["mon", "wed", "fri"]
      },
      {
          "id": 2,
          "state": false,
          "time": "22:00:00",
          "days": ["tue", "thu", "sat", "sun"]
      }
  ]
  ```

### Error Response

- **Status**: 404 Not Found
- **Body**:
  ```json
  {
      "error": "Relay not found"
  }
  ```

## Relay Alarm Rule Creation

### Request

- **Endpoint**: `/api/relay-alarm`
- **Method**: POST
- **Body**:
  ```json
  {
      "relayId": 1,
      "state": true, // or false
      "time": "06:00:00",
      "days": ["mon", "wed", "fri"]
  }
  ```

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Relay alarm rule created successfully",
      "ruleId": 123
  }
  ```

### Error Responses

- **Status**: 400 Bad Request
- **Body**:
  ```json
  {
      "error": "Invalid data provided"
  }
  ```

- **Status**: 404 Not Found
- **Body**:
  ```json
  {
      "error": "Relay not found"
  }
  ```

## Relay Alarm Rule Update

### Request

- **Endpoint**: `/api/relay-alarm/:relayId/:ruleId`
- **Method**: PUT
- **Body**:
  ```json
  {
      "state": false, // or true
      "time": "22:00:00",
      "days": ["tue", "thu", "sat", "sun"]
  }
  ```

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Relay alarm rule updated successfully"
  }
  ```

### Error Responses

- **Status**: 400 Bad Request
- **Body**:
  ```json
  {
      "error": "Invalid data provided"
  }
  ```

- **Status**: 404 Not Found
- **Body**:
  ```json
  {
      "error": "Relay or rule not found"
  }
  ```

## Relay Alarm Rule Deletion

### Request

- **Endpoint**: `/api/relay-alarm/:relayId/:ruleId`
- **Method**: DELETE

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Relay alarm rule deleted successfully"
  }
  ```

### Error Responses

- **Status**: 404 Not Found
- **Body**:
  ```json
  {
      "error": "Relay or rule not found"
  }
  ```

## Network Information Retrieval

### Request

- **Endpoint**: `/api/network-info`
- **Method**: GET

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "wifiMode": "Connected",
      "ipAddress": "123.123.123.123",
      "gateway": "123.123.123.1",
      "dns": "123.123.123.1"
  }
  ```

## Server Time Retrieval

### Request

- **Endpoint**: `/api/server-time`
- **Method**: GET

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
    "time": "11:32:45", 
    "date": "2024-07-23"
  }
  ```

## Firmware Update

### Request

- **Endpoint**: `/api/update-firmware`
- **Method**: POST
- **Content-Type**: `multipart/form-data`
- **Body**: Contains the file data for the firmware (.bin file).

Example using `curl`:
```bash
curl -X POST -F 'firmware=@path_to_firmware_file.bin' http://yourapi.com/api/update-firmware
```

### Successful Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Firmware updated successfully"
  }
  ```

### Error Responses

- **Status**: 400 Bad Request
- **Body**:
  ```json
  {
      "error": "Invalid firmware file"
  }
  ```