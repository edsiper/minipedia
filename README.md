# MiniPedia

This is a simple service implementing multiple editions of an article through
a web service.

Please refer to __API Details__ for a list of the available interfaces.

## API details

Interface | Method | Description |
----------|--------|-----|
/Latest_plane_crash| GET | It retrieve the latest article about a crashed plane   |
/Latest_plane_crash/update| POST | Fo |

## Accessing through Curl

The best way to see how the service behave is to use the _Curl_ program.

### Retrieving the latest article

```
$ curl -i http://166.78.109.6:8080/Latest_plane_crash
```

That command will return the HTTP headers plus the content, pay attention to the headers as they indicate which version is being retrieved:

```
HTTP/1.1 200 OK
Server: Monkey/1.4.0
Date: Thu, 24 Jul 2014 02:53:54 GMT
Content-Length: 2334
X-Minipedia-Version-Id: 1
Content-Type: text/html

<html>
<body>
<head>
  <title>MiniPedia - Latest Plane Crash</title>
  </head>
  <body>

<p>
<strong>(CNN)</strong>
At least 48 people were killed when a twin-engine turboprop plane crashed Wednesday while attempting to land in bad weather in Taiwan's Penghu Islands, according to Taiwan's Central News Agency.
</p>
....
```

### Updating an article

Updating an article requires a POST request as follows:

```
$ curl -i -X POST -d @~/somefile.html http://166.78.109.6:8080/Latest_plane_crash/update
```

Once updated the server will return the new version number assigned and the number of bytes written to the disk:

```
HTTP/1.1 200 OK
Server: Monkey/1.4.0
Date: Thu, 24 Jul 2014 02:54:32 GMT
Content-Length: 36
X-Minipedia-Version-Id: 2

Document updated: 166 bytes written
```

## Updating an article

When a request comes through the _update_ interface, it retrieve the latest ID for the article inside a critical section, it increments the counter and flush back the file to the disk.

The critical section is protected through a Mutex, but of course this is not the right approach, it supposed to work in a database or in a local cache that support this kind of atomic operations.

Author
======
Eduardo Silva P. <edsiper@gmail.com>
