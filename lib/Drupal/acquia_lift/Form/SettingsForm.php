<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Form\SettingsForm.
 */

namespace Drupal\acquia_lift\Form;

use Drupal\acquia_lift\AcquiaLiftAPI;
use Drupal\Core\Form\ConfigFormBase;

/**
 * Form builder for the admin display defaults page.
 */
class SettingsForm extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_lift_settings';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, array &$form_state) {
    $form = parent::buildForm($form, $form_state);

    $config = $this->configFactory()->get('acquia_lift.settings');
    $owner_code = $config->get('owner_code');
    $api_key = $config->get('api_key');
    $admin_key = $config->get('admin_key');
    $api_url = $config->get('api_url');
    $account_info_provided = !empty($owner_code) && !empty($api_key);

    if ($account_info_provided) {
      // Add a button for checking the connection.
      $form['ping_test_wrapper'] = array(
        '#theme_wrappers' => array('container'),
        '#attributes' => array('id' => 'acquia-lift-config-messages'),
      );
      $form['ping_test'] = array(
        '#type' => 'submit',
        '#value' => t('Test connection to Acquia Lift'),
        '#attributes' => array('title' => t('Click here to check your Acquia Lift connection.')),
        '#submit' => array('acquia_lift_ping_test_submit'),
        '#ajax' => array(
          'callback' => 'acquia_lift_ping_test_ajax_callback',
          'wrapper' => 'acquia-lift-ping-test',
          'effect' => 'fade',
        ),
        '#limit_validation_errors' => array(),
      );
      // Add info about number of API calls made last month and current month
      // to date.
      try {
        $api = \Drupal::service('acquia_lift_api');
        $ts = time();
        $calls_last_month = $api->getTotalRuntimeCallsForPreviousMonth($ts);
        $form['calls_last_month'] = array(
          '#type' => 'markup',
          '#markup' => '<div>' . t('Number of API calls made last month: ') . $calls_last_month . '</div>',
        );
        $calls_this_month = $api->getTotalRuntimeCallsForMonthToDate($ts);
        $form['calls_this_month'] = array(
          '#type' => 'markup',
          '#markup' => '<div>' . t('Number of API calls made so far this month: ') . $calls_this_month . '</div>',
        );
      }
      catch (\Exception $e) {
        drupal_set_message($e->getMessage(), 'error');
      }
    }

    $form['acquia_lift_account_info'] = array(
      '#type' => 'fieldset',
      '#title' => 'Acquia Lift Account Settings',
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#collapsed' => $account_info_provided
    );

    $form['acquia_lift_account_info']['msg'] = array(
      '#markup' => t("<p>This information is used to link your !acquialift account to Drupal.</p><p>Email !liftemail to get set up with Acquia Lift credentials.</p>", array('!acquialift' => l(t('Acquia Lift'), 'http://www.acquia.com/products-services/website-personalization', array('attributes' => array('target' => '_blank'))), '!liftemail' => l('lift@acquia.com', 'mailto:lift@acquia.com'))),
    );

    $form['acquia_lift_account_info']['owner_code'] = array(
      '#type' => 'textfield',
      '#title' => t('Owner Code'),
      '#default_value' => !empty($owner_code) ? $owner_code : '',
      '#size' => 35,
      '#maxlength' => 50,
      '#description' => t("Paste in your Acquia Lift owner code"),
      '#required' => TRUE,
    );

    $form['acquia_lift_account_info']['api_key'] = array(
      '#type' => 'textfield',
      '#title' => t('Runtime API Key'),
      '#default_value' => !empty($api_key) ? $api_key : '',
      '#size' => 35,
      '#maxlength' => 50,
      '#description' => t("Paste in your Acquia Lift api key"),
      '#required' => TRUE,
    );

    $form['acquia_lift_account_info']['admin_key'] = array(
      '#type' => 'textfield',
      '#title' => t('Admin API Key'),
      '#default_value' => !empty($admin_key) ? $admin_key : '',
      '#size' => 35,
      '#maxlength' => 50,
      '#description' => t("Paste in your Acquia Lift admin key"),
      '#required' => TRUE,
    );

    $form['acquia_lift_account_info']['api_url'] = array(
      '#type' => 'textfield',
      '#title' => t('API Server URL'),
      '#default_value' => !empty($api_url) ? $api_url : '',
      '#size' => 35,
      '#maxlength' => 50,
      '#description' => t("Paste in your Acquia Lift API URL"),
      '#required' => TRUE,
    );

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, array &$form_state) {
    if (!AcquiaLiftAPI::codeIsValid($form_state['values']['acquia_lift_account_info']['owner_code'])) {
      \Drupal::formBuilder()->setErrorByName('acquia_lift_account_info][owner_code', $form_state, "Invalid owner code");
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, array &$form_state) {
    $this->configFactory()->get('acquia_lift.settings')
      ->set('owner_code', $form_state['values']['acquia_lift_account_info']['owner_code'])
      ->set('api_key', $form_state['values']['acquia_lift_account_info']['api_key'])
      ->set('admin_key', $form_state['values']['acquia_lift_account_info']['admin_key'])
      ->set('api_url', $form_state['values']['acquia_lift_account_info']['api_url'])
      ->save();
  }
}
