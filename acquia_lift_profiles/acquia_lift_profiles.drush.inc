<?php
/**
 * @file acquia_lift_profiles.drush.inc
 * Provides Acquia Lift Profiles drush commands.
 */

/**
 * Implements hook_drush_command().
 */
function acquia_lift_profiles_drush_command() {
  $items['acquia-lift-profiles-get-segments'] = array (
    'description' => 'List segments',
    'aliases' => array('alp-seg'),
    'arguments' => array(
      'accessKey' => 'The access key to use (optional)',
      'secretKey' => 'The secret key to use (optional)',
    ),
  );
  return $items;
}

/**
 * Prints out all the segments in Acquia Lift Web for the configured customer.
 *
 * @param null $accessKey Optional access key to use (if null then the configured access key is used)
 * @param null $secretKey Optional secret key to use (if null then the configured secret key is used)
 */
function drush_acquia_lift_profiles_get_segments($accessKey = NULL, $secretKey = NULL) {
  $account_info = acquia_lift_profiles_get_account_info();
  drupal_set_message(T('Get Segments') );
  drupal_set_message(T('Account Name : ' . $account_info['account_name']));
  drupal_set_message(T('API URL : ' . $account_info['hostname']));

  if ( $accessKey == NULL ) {
    $accessKey = $account_info['public_key'];
  }
  if ( $secretKey == NULL ) {
    $secretKey = $account_info['secret_key'];
  }
  drupal_set_message(T('Access Key : '.$accessKey));
  drupal_set_message(T('Secret Key : '.$secretKey));
  drupal_set_message(T('Segments : '));
  try {
    $returnSegments = ALProfilesAPI::getInstance($account_info['account_name'], variable_get('acquia_lift_profiles_site_name', ''), $account_info['hostname'], $account_info['public_key'], $account_info['secret_key'])
      ->getSegments();
    foreach ($returnSegments as $segment) {
      drupal_set_message($segment);
    }
  } catch ( Exception $e ) {
    drupal_set_message(T($e->getMessage()));
  }
}
