#!/usr/bin/env bash

# Run server.
drush runserver http://127.0.0.1:8080 &
# Run tests.
php ./core/scripts/run-tests.sh --verbose --php $(which php) --url 'http://127.0.0.1:8080' --dburl 'mysql://root:@127.0.0.1/drupal' --module 'acquia_lift'
