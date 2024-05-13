# Smart Relays API Backend

This API backend serves as the interface between the frontend interface for controlling smart relays and the underlying system managing the relay devices. Below are the details of the requests you need to make to interact with the backend and the corresponding responses.

## Get All Relays

### Request

- **Endpoint**: `/api/all-relays`
- **Method**: GET

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  [
      {
          "id": 1,
          "name": "Relay 1",
          "state": "on" // or "off"
      },
      {
          "id": 2,
          "name": "Relay 2",
          "state": "off"
      }
  ]
  ```

## Relay Control

### Request

- **Endpoint**: `/api/relay-control`
- **Method**: POST
- **Body**:
  ```json
  {
      "relayId": 1,
      "state": "on" // or "off"
  }
  ```

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Relay state updated successfully",
      "relayId": 1,
      "currentState": "on" // or "off"
  }
  ```

## Current System Settings Retrieval

### Request

- **Endpoint**: `/api/current-settings`
- **Method**: GET

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "systemName": "Smart Relays",
      "wifiName": "YourWiFiName",
      "systemTime": "11:32:45",
      "systemDate": "2024-07-23",
      "syncTime": true,
      "relayNames": {
          "relay1": "Relay 1",
          "relay2": "Relay 2"
      }
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
      "relayNames": {
        "relay1": "Relay 1",
        "relay2": "Relay 2"
    }
  }
  ```

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "General settings updated successfully"
  }
  ```

## Get All Alarms for a Specific Relay

### Request

- **Endpoint**: `/api/relay-alarms/:relayId`
- **Method**: GET

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  [
      {
          "id": 1,
          "state": "on",
          "time": "06:00:00",
          "days": ["mon", "wed", "fri"]
      },
      {
          "id": 2,
          "state": "off",
          "time": "22:00:00",
          "days": ["tue", "thu", "sat", "sun"]
      }
  ]
  ```

## Relay Alarm Rules Update

### Request

- **Endpoint**: `/api/relay-alarm/:relayId`
- **Method**: POST
- **Body**:
  ```json
  [
      {
          "state": "on", // or "off"
          "time": "06:00:00",
          "days": ["mon", "wed", "fri"]
      },
      {
          "state": "off",
          "time": "22:00:00",
          "days": ["tue", "thu", "sat", "sun"]
      }
  ]
  ```

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "message": "Relay alarm rules updated successfully"
  }
  ```

## Network Information Retrieval

### Request

- **Endpoint**: `/api/network-info`
- **Method**: GET

### Response

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

### Response

- **Status**: 200 OK
- **Body**:
  ```json
  {
      "serverTime": "2024-05-13T10:30:00Z"
  }
  ```