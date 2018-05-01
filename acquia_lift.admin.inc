<?php

/**
 * @file acquia_lift.admin.inc
 * Provides functions needed for the admin UI.
 */


function acquia_lift_admin_form($form, &$form_state) {
  $is_configured = acquia_lift_is_configured();
  $form['intro'] = array(
    '#markup' => t('You can find more info in the <a href="@docs-link" target="_blank">Documentation</a>.', array(
      '@docs-link' => 'https://docs.acquia.com/lift',
    )),
  );
  $form['credentials'] = array(
    '#type' => 'fieldset',
    '#title' => t('Acquia Lift credential'),
    '#collapsible' => $is_configured,
    '#collapsed' => $is_configured,
    '#tree' => FALSE,
  );
  _acquia_lift_admin_build_credential_form($form['credentials'], $form_state);

  $form['data_collection'] = array(
    '#type' => 'vertical_tabs',
    '#title' => t('Data collection settings'),
  );

  $form['identity'] = array(
    '#type' => 'fieldset',
    '#title' => t('Identity'),
    '#group' => 'data_collection',
    '#collapsible' => TRUE,
    '#tree' => FALSE,
    '#weight' => 10,
  );
  _acquia_lift_admin_build_identity_form($form['identity'], $form_state);

  $form['acquia_lift_field_mappings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Field mappings'),
    '#description' => t('Create !taxonomy_link and map to "content section", "content keywords", and "persona" fields.', array(
        '!taxonomy_link' => l(t('Taxonomy Vocabulary'), 'admin/structure/taxonomy'))
    ),
    '#group' => 'data_collection',
    '#collapsible' => TRUE,
    '#tree' => TRUE,
    '#weight' => 20,
  );
  _acquia_lift_admin_build_field_mappings_form($form['acquia_lift_field_mappings'], $form_state);

  $form['acquia_lift_udf_mappings'] = array(
    '#tree' => TRUE,
  );
  $form['acquia_lift_udf_mappings']['person'] = array(
    '#type' => 'fieldset',
    '#title' => t('User person mappings'),
    '#description' => t('Map taxonomy terms to Visitor Profile person fields in Acquia Lift. Select a Taxonomy Reference Field that, if present, will map the value of the specified field to the Acquia Lift Profile for that specific visitor. No options available? Create !taxonomy_vocabulary_link and map the corresponding value.', array(
        '!taxonomy_vocabulary_link' => l(t('Taxonomy Vocabulary'), 'admin/structure/taxonomy'))
    ),
    '#group' => 'data_collection',
    '#collapsible' => TRUE,
    '#weight' => 30,
  );
  _acquia_lift_admin_build_udf_mappings_form($form['acquia_lift_udf_mappings']['person'], $form_state, 'person');

  $form['acquia_lift_udf_mappings']['touch'] = array(
    '#type' => 'fieldset',
    '#title' => t('User touch mappings'),
    '#description' => t('Map taxonomy terms to Visitor Profile touch fields in Acquia Lift. Select a Taxonomy Reference Field that, if present, will map the value of the specified field to the Acquia Lift Profile for that specific visitor. No options available? Create !taxonomy_vocabulary_link and map the corresponding value.', array(
      '!taxonomy_vocabulary_link' => l(t('Taxonomy Vocabulary'), 'admin/structure/taxonomy'))
    ),
    '#group' => 'data_collection',
    '#collapsible' => TRUE,
    '#weight' => 40,
  );
  _acquia_lift_admin_build_udf_mappings_form($form['acquia_lift_udf_mappings']['touch'], $form_state, 'touch');

  $form['acquia_lift_udf_mappings']['event'] = array(
    '#type' => 'fieldset',
    '#title' => t('User event mappings'),
    '#description' => t('Map taxonomy terms to Visitor Profile event fields in Acquia Lift. Select a Taxonomy Reference Field that, if present, will map the value of the specified field to the Acquia Lift Profile for that specific visitor. No options available? Create !taxonomy_vocabulary_link and map the corresponding value.', array(
        '!taxonomy_vocabulary_link' => l(t('Taxonomy Vocabulary'), 'admin/structure/taxonomy'))
    ),
    '#group' => 'data_collection',
    '#collapsible' => TRUE,
    '#weight' => 50,
  );
  _acquia_lift_admin_build_udf_mappings_form($form['acquia_lift_udf_mappings']['event'], $form_state, 'event');

  $form['visibility'] = array(
    '#type' => 'fieldset',
    '#description' => t('Lift will skip data collection on those URLs and their aliases.'),
    '#title' => t('Visibility'),
    '#group' => 'data_collection',
    '#collapsible' => TRUE,
    '#tree' => FALSE,
    '#weight' => 60,
  );
  _acquia_lift_admin_build_visibility_form($form['visibility'], $form_state);

  $form['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced configuration'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => FALSE,
    '#weight' => 70,
  );
  _acquia_lift_admin_build_advanced_confirmation_form($form['advanced'], $form_state);
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#weight' => 80,
  );
  return $form;
}

function _acquia_lift_admin_build_credential_form(&$form, &$form_state) {
  $form['acquia_lift_account_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Account ID'),
    '#description' => t('Your Lift subscription\'s account ID.'),
    '#default_value' => variable_get('acquia_lift_account_id', ''),
    '#required' => TRUE,
  );
  $form['acquia_lift_site_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Site ID'),
    '#description' => t('Current site\'s site ID. WARNING: different sites must use different value here, even between a staging and a production of the same site.'),
    '#default_value' => variable_get('acquia_lift_site_id', ''),
    '#required' => TRUE,
  );
  $form['acquia_lift_assets_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Assets URL'),
    '#description' => t('Your Lift application\'s assets URL.  It determines which version of the Lift application is being used.'),
    '#field_prefix' => 'https://',
    '#default_value' => _acquia_lift_admin_clean_url(variable_get('acquia_lift_assets_url', '')),
    '#required' => TRUE,
  );
  $form['acquia_lift_decision_api_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Decision API URL'),
    '#description' => t('Your Lift Decision API\'s URL.  Unless explicitly instructed, leave empty to use default URL.'),
    '#field_prefix' => 'https://',
    '#default_value' => _acquia_lift_admin_clean_url(variable_get('acquia_lift_decision_api_url', '')),
  );
  $form['acquia_lift_oauth_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Authentication URL'),
    '#description' => t('Your Lift Authentication API\'s URL.  Unless explicitly instructed, leave empty to use default URL.'),
    '#field_prefix' => 'https://',
    '#field_suffix' => '/authorize',
    '#default_value' => _acquia_lift_admin_clean_url(_acquia_lift_admin_remove_authorize_suffix(variable_get('acquia_lift_oauth_url', ''))),
  );
  return $form;
}


/**
 * Admin form for configuring acquia_lift behavior.
 */
function _acquia_lift_admin_build_identity_form(&$form, &$form_state) {
  $identity_parameter_display_value = variable_get('acquia_lift_identity_param', 'identity');
  $identity_type_parameter_display_value = variable_get('acquia_lift_identity_type_param') ?: 'identityType';
  $default_identity_type_display_value = variable_get('acquia_lift_default_identity_type') ?: ACQUIA_LIFT_DEFAULT_IDENTITY_TYPE_IDENTIFIER;
  $default_identity_type_default_value = variable_get('acquia_lift_default_identity_type') ?: ACQUIA_LIFT_DEFAULT_IDENTITY_TYPE_DEFAULT;

  $form['acquia_lift_identity_param'] = array(
    '#type' => 'textfield',
    '#title' => t('Identity Parameter'),
    '#description' => t('The URL link parameter for specific visitor information, such as an email address or social media username, which is sent to the Lift Profile Manager. Example using <strong>@identity_parameter_display_value</strong>: ?<strong><ins>@identity_parameter_display_value</ins></strong>=jdoe01', [
      '@identity_parameter_display_value' => $identity_parameter_display_value,
    ]),
    '#default_value' => variable_get('acquia_lift_identity_param'),
  );
  $form['acquia_lift_identity_type_param'] = array(
    '#type' => 'textfield',
    '#title' => t('Identity Type Parameter'),
    '#description' => t('The URL link parameter that corresponds to a Lift Profile Manager identifier type (one of the pre-defined ones or a new one you\'ve created). Example using <strong>@identity_type_parameter_display_value</strong>: ?@identity_parameter_display_value=jdoe01&<strong><ins>@identity_type_parameter_display_value</ins></strong>=@default_identity_type_default_value', [
      '@identity_parameter_display_value' => $identity_parameter_display_value,
      '@identity_type_parameter_display_value' => $identity_type_parameter_display_value,
      '@default_identity_type_default_value' => $default_identity_type_default_value,
    ]),
    '#default_value' => variable_get('acquia_lift_identity_type_param'),
    '#states' => array(
      'visible' => array(
        ':input[name="acquia_lift_identity_param"]' => ['!value' => ''],
      ),
    ),
  );
  $form['acquia_lift_default_identity_type'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Identity Type'),
    '#description' => t('The Lift Profile Manager identity type to be used by default. Example using <strong>@default_identity_type_display_value</strong>: a visitor may visit the site through ?@identity_parameter_display_value=jdoe01 and omit the "@identity_type_parameter_display_value" query, and Lift will automatically identify this visitor as "jdoe01" of <strong><ins>@default_identity_type_display_value</ins></strong> type. Leave this field blank to default to <strong>@default</strong> identity type."', [
      '@default' => 'email',
      '@identity_parameter_display_value' => $identity_parameter_display_value,
      '@identity_type_parameter_display_value' => $identity_type_parameter_display_value,
      '@default_identity_type_display_value' => $default_identity_type_display_value,
    ]),
    '#default_value' => variable_get('acquia_lift_default_identity_type'),
    '#states' => array(
      'visible' => array(
        ':input[name="acquia_lift_identity_param"]' => ['!value' => ''],
      ),
    ),
    '#attributes' => array(
      'placeholder' => ACQUIA_LIFT_DEFAULT_IDENTITY_TYPE_DEFAULT,
    ),
  );
}

function _acquia_lift_admin_build_field_mappings_form(&$form, &$form_state) {
  // Retrieving all context options.
  $groups = acquia_lift_get_grouped_context_options();

  $field_list = array(
    'content_section' => 'Content Section',
    'content_keywords' => 'Content Keywords',
    'persona' => 'Persona',
  );
  $field_mappings = variable_get('acquia_lift_field_mappings', array());
  foreach ($field_list as $field_name => $field_friendly_name) {
    $default_value = isset($field_mappings[$field_name]) ? $field_mappings[$field_name] : NULL;
    $select = _acquia_lift_admin_build_visitor_context_select($groups, $default_value);
    $form[$field_name] = $select;
    $form[$field_name]['#title'] = t($field_friendly_name);
  }
}

function _acquia_lift_admin_build_udf_mappings_form(&$form, &$form_state, $udf_type) {
  $udf_mappings = variable_get('acquia_lift_udf_mappings', array());
  $udf_options = acquia_lift_get_udfs($udf_type);

  $groups = acquia_lift_get_grouped_context_options();

  foreach($udf_options as $i => $udf_title) {
    $default_value = isset($udf_mappings[$udf_type][$udf_title]) ? $udf_mappings[$udf_type][$udf_title] : NULL;
    $select = _acquia_lift_admin_build_visitor_context_select($groups, $default_value);
    $select['#title'] = t('User profile @type field @number', array(
      '@type' => $udf_type,
      '@number' => $i + 1,
    ));
    $form[$udf_title] = $select;
  }
}

function _acquia_lift_admin_build_visibility_form(&$form, &$form_state) {
  $form['acquia_lift_ignore_path_patterns'] = array(
    '#type' => 'textarea',
    '#title' => t('Path patterns'),
    '#default_value' => variable_get('acquia_lift_ignore_path_patterns', "/admin\n/admin/*\n/batch\n/node/add*\n/node/*/*\n/user/*\n/block/*"),
  );
}

function _acquia_lift_admin_build_advanced_confirmation_form(&$form, &$form_state) {
  $form['acquia_lift_content_replacement_mode'] = [
    '#type' => 'radios',
    '#title' => t('Content replacement mode'),
    '#description' => t('The default, site-wide setting for <a href="@docs-link" target="_blank">content replacement mode</a>.', array(
      '@docs-link' => 'https://docs.acquia.com/lift/drupal/3/config/trusted',
    )),
    '#default_value' => variable_get('acquia_lift_content_replacement_mode', 'untrusted'),
    '#options' => [
      'trusted' => t('Trusted'),
      'untrusted' => t('Untrusted'),
      'customized' => t('Customized')
    ],
  ];
  $form['acquia_lift_content_origin'] = array(
    '#type' => 'textfield',
    '#title' => t('Content Hub Origin Site UUID'),
    '#description' => t('Show content in Experience Builder content list from only one origin site, specified by its Content Hub Site UUID. Leave empty to show content from all sites.'),
    '#default_value' => variable_get('acquia_lift_content_origin'),
  );
}

/**
 * Returns a visitor context single select box.
 */
function _acquia_lift_admin_build_visitor_context_select($groups, $default_value) {
  if (count($groups) == 0) {
    return FALSE;
  }
  $element = array(
    '#type' => 'select',
    '#empty_option' => t('- Not mapped -'),
    '#options' => $groups,
    '#default_value' => $default_value,
  );
  return $element;
}

function acquia_lift_admin_form_validate($form, &$form_state) {
  if (!preg_match('/^[a-zA-Z_][a-zA-Z\\d_]*$/', $form_state['values']['acquia_lift_account_id'])) {
    form_set_error('acquia_lift_account_id', t('Account ID contains invalid characters.  It has to start with a letter and contain only alphanumerical characters.'));
  }
  if (!empty($form_state['values']['acquia_lift_assets_url']) && !valid_url($form_state['values']['acquia_lift_assets_url'])) {
    form_set_error('acquia_lift_assets_url', t('Assets URL is an invalid URL.'));
  }
  if (!empty($form_state['values']['acquia_lift_decision_api_url']) && !valid_url($form_state['values']['acquia_lift_decision_api_url'])) {
    form_set_error('acquia_lift_decision_api_url', t('Decision API URL is an invalid URL.'));
  }
  if (!empty($form_state['values']['acquia_lift_oauth_url']) && !valid_url($form_state['values']['acquia_lift_oauth_url'])) {
    form_set_error('acquia_lift_oauth_url', t('Authentication URL is an invalid URL.'));
  }
  if (!empty($form_state['values']['acquia_lift_content_origin']) && !(bool) preg_match('/^[0-9a-f]{8}-([0-9a-f]{4}-){3}[0-9a-f]{12}$/', $form_state['values']['acquia_lift_content_origin'])) {
    form_set_error('acquia_lift_content_origin', t('Content Hub Origin Site UUID is not a valid UUID.'));
  }
}

/**
 * Submit handler for the acquia_lift configuration form.
 */
function acquia_lift_admin_form_submit($form, &$form_state) {
  // Field mappings.
  if (isset($form_state['values']['acquia_lift_field_mappings'])) {
    // Don't save empty entries.
    $acquia_lift_field_mappings = array_filter($form_state['values']['acquia_lift_field_mappings']);
    variable_set('acquia_lift_field_mappings', $acquia_lift_field_mappings);
  }

  if (isset($form_state['values']['acquia_lift_udf_mappings'])) {
    $udf_types = acquia_lift_get_udf_types();
    $acquia_lift_udf_mappings = array();
    foreach($udf_types as $type => $count) {
      if (isset($form_state['values']['acquia_lift_udf_mappings'][$type])) {
        $acquia_lift_udf_mappings[$type] = array_filter($form_state['values']['acquia_lift_udf_mappings'][$type]);
      }
    }
    $acquia_lift_udf_mappings = array_filter($acquia_lift_udf_mappings);
    variable_set('acquia_lift_udf_mappings', $acquia_lift_udf_mappings);
  }

  // Enforce https and clean up the urls
  $assets_url = $form_state['values']['acquia_lift_assets_url'];
  $decision_api_url = $form_state['values']['acquia_lift_decision_api_url'];
  $oauth_url = $form_state['values']['acquia_lift_oauth_url'];
  if (!empty($assets_url)) {
    $assets_url = 'https://' . _acquia_lift_admin_clean_url($assets_url);
    _acquia_lift_ping('Assets', $assets_url);
  }
  if (!empty($decision_api_url)) {
    $decision_api_url = 'https://' . _acquia_lift_admin_clean_url($decision_api_url);
    _acquia_lift_ping('Decision API', $decision_api_url);
  }
  if (!empty($oauth_url)) {
    $oauth_url = 'https://' . _acquia_lift_admin_remove_authorize_suffix(_acquia_lift_admin_clean_url($oauth_url)) . '/authorize';
    _acquia_lift_ping('Authentication', $oauth_url);
  }
  variable_set('acquia_lift_assets_url', $assets_url);
  variable_set('acquia_lift_decision_api_url', $decision_api_url);
  variable_set('acquia_lift_oauth_url', $oauth_url);

  // The rest of the variables are set as is.
  $standard_variables = array(
    'acquia_lift_identity_param',
    'acquia_lift_identity_type_param',
    'acquia_lift_default_identity_type',
    'acquia_lift_account_id',
    'acquia_lift_site_id',
    'acquia_lift_ignore_path_patterns',
    'acquia_lift_content_replacement_mode',
    'acquia_lift_content_origin',
  );
  foreach ($standard_variables as $key) {
    variable_set($key, $form_state['values'][$key]);
  }

  drupal_set_message(t('The configuration settings have been saved.'), 'status');
}

/**
 * Clean up URL. Remove the:
 *   1) Protocol "http://" and "http://".
 *   2) Leading and trailing slashes and space characters.
 *
 * @param string $url
 *   URL.
 * @return string
 *   URL, but cleaned up.
 */
function _acquia_lift_admin_clean_url($url) {
  $searchFor = [
    '~^[ \t\r\n\/]+~',
    '~[ \t\r\n\/]+$~',
    '~^https?://~',
  ];
  return preg_replace($searchFor, '', $url);
}

/**
 * Remove the "/authorize" suffix, if any.
 *
 * @param string $url
 *   URL.
 * @return string
 *   URL, but with "/authorize" removed.
 */
function _acquia_lift_admin_remove_authorize_suffix($url) {
  return preg_replace('~/authorize$~', '', $url);
}

/**
 * Pings a url and validates that the URL can be reached and shows messaging
 * if it cannot..
 * NOTE: the drupal 8 version of this functionality also checks the response
 * code (expecting 200) but this does not seem possible as responses here
 * are correctly 403, 400, etc.
 *
 * @param string $type
 *   The type of the URL to check
 * @param string $url
 *   The URL to check
 * @return bool
 *   True if reachable, false otherwise.
 */
function _acquia_lift_ping($type, $url) {
  $ch = curl_init($url);
  curl_setopt($ch, CURLOPT_TIMEOUT, 5);
  curl_setopt($ch, CURLOPT_NOBODY, TRUE);
  curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 5);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
  curl_exec($ch);
  $status_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
  curl_close($ch);
  if ($status_code == 0) {
    drupal_set_message(t('Acquia Lift module could not reach the specified @type URL.', array(
      '@type' => $type,
    )), 'error');
    return FALSE;
  }
  return TRUE;
}
