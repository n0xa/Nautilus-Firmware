<?php
// Simple CORS proxy for GitHub release assets
// Usage: proxy.php?url=https://github.com/.../releases/download/.../file.bin

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

$url = $_GET['url'] ?? '';

// Validate it's a GitHub release URL
if (!preg_match('#^https://github\.com/[^/]+/[^/]+/releases/download/#', $url)) {
    http_response_code(400);
    echo json_encode(['error' => 'Invalid URL']);
    exit();
}

// Fetch the file from GitHub
$ch = curl_init($url);
curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_HEADER, false);
curl_setopt($ch, CURLOPT_USERAGENT, 'Nautilus-Webflasher-Proxy/1.0');

$data = curl_exec($ch);
$httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
$contentType = curl_getinfo($ch, CURLINFO_CONTENT_TYPE);
curl_close($ch);

if ($httpCode !== 200) {
    http_response_code($httpCode);
    echo json_encode(['error' => 'Failed to fetch from GitHub']);
    exit();
}

header('Content-Type: ' . $contentType);
echo $data;
?>
