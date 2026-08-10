<?php
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.php                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: erpascua <erpascua@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 13:37:19 by erpascua          #+#    #+#             */
/*   Updated: 2026/06/11 16:19:09 by erpascua         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

$method = getenv('REQUEST_METHOD') ?: '';

function print_var($s)
{
    return htmlspecialchars((string) $s, ENT_QUOTES, 'UTF-8');
}

$count = isset($_COOKIE['hits_php']) ? ((int) $_COOKIE['hits_php']) + 1 : 1;
setcookie('hits_php', (string) $count, 0, '/cgi.php');

header('Content-Type: text/html');

function print_params($params)
{
    if (empty($params)) {
        echo "<p class=\"empty\">No parameter received</p>\n";
        return;
    }
    ksort($params);
    echo "<ul class=\"kv\">\n";
    foreach ($params as $key => $val) {
        echo "  <li><span class=\"k\">" . print_var($key) . "</span>"
            . "<span class=\"v\">" . print_var($val) . "</span></li>\n";
    }
    echo "</ul>\n";
}

function print_form($verb)
{
    $id = strtolower($verb);

    echo "<section class=\"card\">\n";
    echo "<h2><span class=\"verb\">$verb</span>Send a $verb</h2>\n";
    echo "<p class=\"hint\">The parameters come back parsed in the cards above.</p>\n";
    echo "<form method=\"$verb\" action=\"/cgi.php\">\n";
    echo "<label for=\"$id-msg\">Message</label>\n";
    echo "<div class=\"row\">\n";
    echo "<input id=\"$id-msg\" type=\"text\" name=\"message\" value=\"hello\">\n";
    echo "<button type=\"submit\">Send</button>\n";
    echo "</div>\n";
    echo "</form>\n";
    echo "</section>\n";
}

echo "<!DOCTYPE html>\n";
echo "<html lang=\"en\">\n";
echo "<head>\n";
echo "<meta charset=\"utf-8\">\n";
echo "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
echo "<title>PHP CGI &mdash; The Tonton Webserv</title>\n";
echo "<link rel=\"stylesheet\" href=\"/style.css\">\n";
echo "</head>\n";
echo "<body>\n";

echo "<header class=\"site\">\n";
echo "<h1>PHP CGI</h1>\n";
echo "<p>Welcome to our fabulous PHP CGI!</p>\n";
echo "<div class=\"pills\">\n";
echo "<span class=\"pill\">" . print_var($method ?: 'GET') . "</span>\n";
echo "<span class=\"pill\">visit #" . print_var($count) . "</span>\n";
echo "<span class=\"pill\">" . print_var(getenv('SERVER_PROTOCOL')) . "</span>\n";
echo "</div>\n";
echo "</header>\n";

echo "<main>\n";

echo "<section class=\"card\">\n";
echo "<h2>Query string</h2>\n";
echo "<p class=\"hint\">Read from <code>QUERY_STRING</code>.</p>\n";
print_params($_GET);
echo "</section>\n";

echo "<section class=\"card\">\n";
echo "<h2>Request body</h2>\n";
echo "<p class=\"hint\">Read from <code>stdin</code>, sized by <code>CONTENT_LENGTH</code>.</p>\n";
print_params($_POST);
echo "</section>\n";

echo "<section class=\"card\">\n";
echo "<h2>CGI environment</h2>\n";
echo "<p class=\"hint\">A few of the variables webserv exports to the script.</p>\n";
echo "<ul class=\"kv\">\n";
foreach (
    array('REQUEST_METHOD', 'SCRIPT_NAME', 'SERVER_NAME', 'SERVER_PORT',
        'SERVER_PROTOCOL', 'CONTENT_LENGTH', 'GATEWAY_INTERFACE', 'SERVER_SOFTWARE') as $name
) {
    $value = getenv($name);
    echo "  <li><span class=\"k\">" . print_var($name) . "</span>"
        . "<span class=\"v\">" . print_var($value !== false && $value !== '' ? $value : '-') . "</span></li>\n";
}
echo "</ul>\n";
echo "</section>\n";

print_form('GET');
print_form('POST');

echo "<section class=\"card wide\">\n";
echo "<h2>Elsewhere</h2>\n";
echo "<ul class=\"links\">\n";
echo "<li><a href=\"/\"><code>/</code><span>Back to the home page</span></a></li>\n";
echo "<li><a href=\"/cgi.py\"><code>/cgi.py</code><span>The same page in Python</span></a></li>\n";
echo "<li><a href=\"/upload/\"><code>/upload/</code><span>Uploaded files</span></a></li>\n";
echo "</ul>\n";
echo "</section>\n";

echo "</main>\n";

echo "<footer class=\"site\"><p>42 project &mdash; erpascua &amp; fmotte</p></footer>\n";
echo "</body>\n</html>\n";
