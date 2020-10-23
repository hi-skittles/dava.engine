import httplib
import sys

def get_status_code(host, path="/"):
    """ This function retreives the status code of a website by requesting
        HEAD data from the host. This means that it only requests the headers.
        If the host cannot be reached or something else goes wrong, it returns
        None instead.
    """
    try:
        conn = httplib.HTTPConnection(host)
        conn.request("HEAD", path)
        return conn.getresponse().status
    except StandardError:
        return None


if get_status_code("127.0.0.1:2424", "/packs/stop_local_http_server") == 404:
    sys.exit(status=None)
else:
    sys.exit(status=1)
