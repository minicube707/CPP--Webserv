import os
import sys

from html import escape
from urllib.parse import parse_qsl

method = os.getenv('REQUEST_METHOD') or ''
queries = os.getenv('QUERY_STRING') or ''


def get_cookie(name):
    raw = os.getenv('HTTP_COOKIE') or ''
    for part in raw.split(';'):
        if '=' in part:
            key, value = part.split('=', 1)
            if key.strip() == name:
                return value.strip()
    return ''


previous = get_cookie('hits_py')
count = int(previous) + 1 if previous.isdigit() else 1


def read_post_body():
    if method != 'POST':
        return ''
    try:
        length = int(os.getenv('CONTENT_LENGTH') or 0)
    except ValueError:
        length = 0
    if length <= 0:
        return ''
    return sys.stdin.read(length)


def print_params(raw):
    pairs = parse_qsl(raw, keep_blank_values=True)
    if not pairs:
        print('<p class="empty">No parameter received.</p>')
        return
    print('<ul class="kv">')
    for key, value in pairs:
        print('<li><span class="k">%s</span><span class="v">%s</span></li>'
              % (escape(key), escape(value)))
    print('</ul>')


def print_form(verb):
    print('<section class="card">')
    print('<h2><span class="verb">%s</span>Send a %s</h2>' % (verb, verb))
    print('<p class="hint">The parameters come back parsed in the cards above.</p>')
    print('<form method="%s" action="/cgi.py">' % verb)
    print('<label for="%s-msg">Message</label>' % verb.lower())
    print('<div class="row">')
    print('<input id="%s-msg" type="text" name="message" value="hello">' % verb.lower())
    print('<button type="submit">Send</button>')
    print('</div>')
    print('</form>')
    print('</section>')


def main():
    body = read_post_body()

    print('Content-Type: text/html')
    print('Set-Cookie: hits_py=%d; Path=/cgi.py' % count)
    print()

    print('<!DOCTYPE html>')
    print('<html lang="en">')
    print('<head>')
    print('<meta charset="utf-8">')
    print('<meta name="viewport" content="width=device-width, initial-scale=1.0">')
    print('<title>Python CGI &mdash; The Tonton Webserv</title>')
    # feuille de style statique, servie par le serveur a cette page generee
    print('<link rel="stylesheet" href="/style.css">')
    print('</head>')
    print('<body>')

    print('<header class="site">')
    print('<h1>Python CGI</h1>')
    print('<p>Welcome to our fabulous Python PHP!</p>')
    print('<div class="pills">')
    print('<span class="pill">%s</span>' % escape(method or 'GET'))
    print('<span class="pill">visit #%d</span>' % count)
    print('<span class="pill">%s</span>' % escape(os.getenv('SERVER_PROTOCOL') or ''))
    print('</div>')
    print('</header>')

    print('<main>')

    print('<section class="card">')
    print('<h2>Query string</h2>')
    print('<p class="hint">Read from <code>QUERY_STRING</code>.</p>')
    print_params(queries)
    print('</section>')

    print('<section class="card">')
    print('<h2>Request body</h2>')
    print('<p class="hint">Read from <code>stdin</code>, sized by <code>CONTENT_LENGTH</code>.</p>')
    print_params(body)
    print('</section>')

    print('<section class="card">')
    print('<h2>CGI environment</h2>')
    print('<p class="hint">A few of the variables webserv exports to the script.</p>')
    print('<ul class="kv">')
    for name in ('REQUEST_METHOD', 'SCRIPT_NAME', 'SERVER_NAME', 'SERVER_PORT',
                 'SERVER_PROTOCOL', 'CONTENT_LENGTH', 'GATEWAY_INTERFACE', 'SERVER_SOFTWARE'):
        print('<li><span class="k">%s</span><span class="v">%s</span></li>'
              % (name, escape(os.getenv(name) or '-')))
    print('</ul>')
    print('</section>')

    print_form('GET')
    print_form('POST')

    print('<section class="card wide">')
    print('<h2>Elsewhere</h2>')
    print('<ul class="links">')
    print('<li><a href="/"><code>/</code><span>Back to the home page</span></a></li>')
    print('<li><a href="/cgi.php"><code>/cgi.php</code><span>The same page in PHP</span></a></li>')
    print('<li><a href="/upload/"><code>/upload/</code><span>Uploaded files</span></a></li>')
    print('</ul>')
    print('</section>')

    print('</main>')

    print('<footer class="site"><p>42 project &mdash; erpascua &amp; fmotte</p></footer>')
    print('</body>')
    print('</html>')


main()
