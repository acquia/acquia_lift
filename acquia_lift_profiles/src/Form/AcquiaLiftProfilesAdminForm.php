<?php /**
 * @file
 * Contains \Drupal\acquia_lift_profiles\Form\AcquiaLiftProfilesAdminForm.
 */

namespace Drupal\acquia_lift_profiles\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;

class AcquiaLiftProfilesAdminForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_lift_profiles_admin_form';
  }

/**
 * Admin form for configuring acquia_lift_profiles behavior.
 */
function buildForm(array $form, FormStateInterface &$form_state) {
  $form['#attached']['js'][] = drupal_get_path('module', 'acquia_lift_profiles') . '/js/acquia_lift_profiles.admin.js';
  $form['acquia_lift_profiles_account_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Profiles Account Name'),
    '#default_value' => \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_account_name'),
    '#required' => TRUE,
  );
  $form['acquia_lift_profiles_api_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Profiles API URL'),
    '#field_prefix' => 'http(s)://',
    '#default_value' => \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_api_url'),
    '#required' => TRUE,
  );
  $form['acquia_lift_profiles_access_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Profiles API Access Key'),
    '#default_value' => \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_access_key'),
    '#required' => TRUE,
  );
  $form['acquia_lift_profiles_secret_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Profiles API Secret Key'),
    '#default_value' => \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_secret_key'),
    '#required' => TRUE,
  );
  $form['acquia_lift_profiles_js_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Profiles JavaScript path'),
    '#field_prefix' => 'http(s)://',
    '#default_value' => \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_js_path'),
    '#required' => TRUE,
  );

  $form['acquia_lift_profiles_capture_identity'] = array(
    '#type' => 'checkbox',
    '#title' => t('Capture Identity'),
    '#description' => t('Check this if you want Acquia Lift Profiles to capture the identity of the user upon login or registration. This means sending their email address to the Acquia Lift Profile Manager.'),
    '#default_value' => \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_capture_identity'),
  );
  $vocabularies = taxonomy_get_vocabularies();
  $mappings = \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_vocabulary_mappings');
  $vocab_options = array('' => t('Select...'));
  foreach ($vocabularies as $vid => $voc) {
    $vocab_options[$vid] = $voc->name;
  }
  $form['acquia_lift_profiles_vocabulary_mappings'] = array(
    '#title' => t('Vocabulary Mappings'),
    '#type' => 'fieldset',
    '#tree' => TRUE,
  );
  $form['acquia_lift_profiles_vocabulary_mappings']['content_section'] = array(
    '#title' => t('Content Section'),
    '#description' => t('The vocabulary to use for the content section'),
    '#type' => 'select',
    '#options' => $vocab_options,
    '#default_value' => isset($mappings['content_section']) ? $mappings['content_section'] : '',
  );
  $form['acquia_lift_profiles_vocabulary_mappings']['content_keywords'] = array(
    '#title' => t('Content Keywords'),
    '#description' => t('The vocabulary to use for the content keywords'),
    '#type' => 'select',
    '#options' => $vocab_options,
    '#default_value' => isset($mappings['content_keywords']) ? $mappings['content_keywords'] : '',
  );
  $form['acquia_lift_profiles_vocabulary_mappings']['persona'] = array(
    '#title' => t('Persona'),
    '#description' => t('The vocabulary to use for the persona'),
    '#type' => 'select',
    '#options' => $vocab_options,
    '#default_value' => isset($mappings['persona']) ? $mappings['persona'] : '',
  );
  // Only show the "Tracked actions" selector if acquia_lift_profiles has already been
  // configured for API connections.
  if (acquia_lift_profiles_is_configured(TRUE)) {
    $action_settings = \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_tracked_actions');
    $actions = visitor_actions_get_actions();
    $options = array();
    foreach ($actions as $name => $info) {
      $options[$name] = $info['label'];
    }
    $form['acquia_lift_profiles_tracked_actions'] = array(
      '#title' => t('Actions to track for Acquia Lift Profiles'),
      '#type' => 'select',
      '#options' => $options,
      '#multiple' => TRUE,
      '#default_value' => $action_settings,
      '#size' => count($options),
    );
    module_load_include('inc', 'personalize', 'personalize.admin');
    $groups = personalize_get_grouped_context_options(NULL, TRUE, array('acquia_lift_profiles_context', 'acquia_lift_context'));
    $udf_mappings = \Drupal::config('acquia_lift_profiles.settings')->get('acquia_lift_profiles_udf_mappings');
    $form['acquia_lift_profiles_udf_mappings'] = array(
      '#title' => t('User Defined Field Mappings'),
      '#description' => t('For each user defined field available in Acquia Lift Profiles, you can map a visitor context, whose value will then be sent as the value for that user defined field.'),
      '#type' => 'fieldset',
      '#tree' => TRUE,
    );
    foreach (acquia_lift_profiles_get_udfs() as $type => $udfs) {
      $form['acquia_lift_profiles_udf_mappings'][$type] = array(
        '#title' => t('@type mappings', array('@type' => ucfirst($type))),
        '#type' => 'fieldset',
        '#tree' => TRUE,
      );
      // Start by just showing 3 of each type to map, or if more have already
      // been mapped show that number plus 1.
      $max = count($udfs);
      $min = 3;
      if (isset($udf_mappings[$type])) {
        if (count($udf_mappings[$type]) < $max) {
          $min = max(array(count($udf_mappings[$type]) + 1, 3));
        }
        else {
          $min = $max;
        }
      }
      foreach ($udfs as $i => $udf) {

        $default_value = isset($udf_mappings[$type][$udf]) ? $udf_mappings[$type][$udf] : NULL;
        $select = acquia_lift_profiles_admin_build_visitor_context_select($groups, $default_value);
        $select['#title'] = t('Context to map to the @field field', array('@field' => $udf));
        $form['acquia_lift_profiles_udf_mappings'][$type][$udf] = $select;
        if ($i == $min) {
          $form['acquia_lift_profiles_udf_mappings'][$type][$udf]['#prefix'] = '<div class="acquia-lift-profiles-hidden-udfs">';
        }
        elseif ($i == $max - 1) {
          $form['acquia_lift_profiles_udf_mappings'][$type][$udf]['#suffix'] = '</div>';
        }
      }
      if ($min < $max) {
        $form['acquia_lift_profiles_udf_mappings'][$type]['button__show_all'] = array(
          '#attributes' => array(
            'class' => array('acquia-lift-profiles-udf-show-all'),
          ),
          '#type' => 'button',
          '#value' => t('Show all')
        );
      }
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

}
