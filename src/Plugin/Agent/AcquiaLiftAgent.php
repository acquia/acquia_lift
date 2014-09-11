<?php

namespace Drupal\acquia_lift\Plugin\Agent;

use Drupal\Core\Form\FormStateInterface;
use Drupal\personalize\AgentBase;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Defines an Acquia Lift agent.
 *
 * @Agent(
 *   id = "acquia_lift"
 * )
 */
class AcquiaLiftAgent extends AgentBase {
  public static function create(ContainerInterface $container, array $configuration, $plugin_id, $plugin_definition) {
    return new static($configuration['name'], $configuration['label'], $configuration['configuration'], $plugin_id, $plugin_definition, $container->get('AcquiaLiftAPI'));
  }

  /**
   * Overrides Drupal\personalize\AgentBase::__construct().
   */
  public function __construct($agent_name, $agent_label, array $configuration, $plugin_id, $plugin_definition, $lift_api) {
    parent::__construct($agent_name, $agent_label, $configuration, $plugin_id, $plugin_definition);
    $this->lift_api = $lift_api;
  }

  public function getAssets() {
    return array();
  }

  /**
   * {@inheritdoc}
   *
   * Creates a generic configuration form for all agent types. Individual
   * block plugins can add elements to this form by overriding
   * BlockBase::blockForm(). Most block plugins should not override this
   * method unless they need to alter the generic form elements.
   *
   * @see \Drupal\block\BlockBase::blockForm()
   */
  public function buildConfigurationForm(array $form, FormStateInterface $form_state) {
    $form['moar_stuff'] = array(
      '#title' => 'err',
      '#type' => 'textfield',
      '#default_value' => isset($this->configuration['moar_stuff']) ? $this->configuration['moar_stuff'] : ''
    );
    return $form;
  }

  /**
   * {@inheritdoc}
   *
   * Most block plugins should not override this method. To add validation
   * for a specific block type, override BlockBase::blockValdiate().
   *
   * @see \Drupal\block\BlockBase::blockValidate()
   */
  public function validateConfigurationForm(array &$form, FormStateInterface $form_state) {

  }

  /**
   * {@inheritdoc}
   *
   * Most block plugins should not override this method. To add submission
   * handling for a specific block type, override BlockBase::blockSubmit().
   *
   * @see \Drupal\block\BlockBase::blockSubmit()
   */
  public function submitConfigurationForm(array &$form, FormStateInterface $form_state) {
/*    // Process the block's submission handling if no errors occurred only.
    if (!form_get_errors($form_state)) {
      $this->configuration['label'] = $form_state['values']['label'];
      $this->configuration['label_display'] = $form_state['values']['label_display'];
      $this->configuration['module'] = $form_state['values']['module'];
      $this->blockSubmit($form, $form_state);
    }*/
  }

  public static function saveCallback($agent_data, $new = TRUE) {

  }
}
