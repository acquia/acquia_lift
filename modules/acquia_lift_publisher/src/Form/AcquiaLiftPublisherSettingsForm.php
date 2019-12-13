<?php

namespace Drupal\acquia_lift_publisher\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Class AcquiaLiftPublisherSettingsForm.
 *
 * @package Drupal\acquia_lift_publisher\Form
 */
class AcquiaLiftPublisherSettingsForm extends ConfigFormBase {

  /**
   * Holds the setting configuration ID.
   */
  public const CONFIG_NAME = 'acquia_lift_publisher.settings';

  /**
   * Holds form field name of push setting.
   *
   * @var string
   */
  public static $pushSettingField = 'personalized_content_only';

  /**
   * {@inheritDoc}
   */
  protected function getEditableConfigNames() {
    return [self::CONFIG_NAME];
  }

  /**
   * {@inheritDoc}
   */
  public function getFormId() {
    return 'acquia_lift_publisher_settings';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $form = parent::buildForm($form, $form_state);
    $config = $this->config(self::CONFIG_NAME);

    $form['sync_settings'] = [
      '#type' => 'fieldset',
      '#title' => $this->t('Synchronization'),
    ];

    $form['sync_settings'][static::$pushSettingField] = [
      '#type' => 'checkbox',
      '#title' => $this->t('Push personalizing content only'),
      '#description' => $this->t('Check this option if this site is used for pushing content to Acquia Lift; uncheck this option if this site is used for pushing content to both Acquia Lift and Content Hub. (default on)'),
      '#default_value' => $config->get(static::$pushSettingField) ?? TRUE,
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $config = $this->configFactory->getEditable(self::CONFIG_NAME);
    $config->set(static::$pushSettingField, $form_state->getValue(static::$pushSettingField));
    $config->save();

    parent::submitForm($form, $form_state);
  }

}
