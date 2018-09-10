#!/usr/bin/env bash

# Upgrade PHPUnit to 6+ if PHP version is 7+.
if [ $(php -v | head -n 1 | cut -d ' ' -f 2 | cut -d '.' -f 1) -ge 7 ]; then
  composer run-script drupal-phpunit-upgrade;
fi
# Run server.
drush runserver http://127.0.0.1:8080 &
# Run tests.
php ./core/scripts/run-tests.sh --verbose --php $(which php) --url 'http://127.0.0.1:8080' --dburl 'mysql://root:@127.0.0.1/drupal' --module 'acquia_lift'
