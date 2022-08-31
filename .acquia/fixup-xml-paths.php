<?php

/**
 * @file
 * Script to fix paths in the clover coverage and JUnit files.
 */

declare(strict_types=1);

$file = $argv[1] ?? '';
if ($file === '') {
  print 'Must provide a file path.' . PHP_EOL;
  exit(1);
}
if (!file_exists($file)) {
  print sprintf('File does not exist at %s.', $file) . PHP_EOL;
  exit(1);
}

$content = file_get_contents($file);
if ($content === FALSE) {
  print sprintf('Could not read file at %s.', $file) . PHP_EOL;
  exit(1);
}
$content = str_replace('/ramfs', '', $content);
file_put_contents($file, $content);
