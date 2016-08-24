<?php

namespace Drupal\acquia_lift\Form;

use Acquia\LiftClient\Entity\Visibility;
use Drupal\acquia_lift\AcquiaLiftException;
use Drupal\Core\Entity\EntityForm;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Provides a form for configuring the processors of a facet.
 */
class SlotForm extends EntityForm {

  /**
   * The facet being configured.
   *
   * @var \Drupal\acquia_lift\SlotInterface
   */
  protected $slot;

  /**
   * The slot storage.
   *
   * @var \Drupal\Core\Entity\EntityStorageInterface
   */
  protected $slotStorage;

  /**
   * The entity manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The Lift API Helper.
   *
   * @var \Acquia\LiftClient\Lift
   */
  protected $liftClient;

  /**
   * Constructs an FacetDisplayForm object.
   *
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity manager.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager) {
    $this->entityTypeManager = $entity_type_manager;
    $this->slotStorage = $entity_type_manager->getStorage('acquia_lift_slot');
    try {
      /** @var \Drupal\acquia_lift\Service\Helper\LiftAPIHelper $liftHelper */
      $liftHelper =  \Drupal::getContainer()->get('acquia_lift.service.helper.lift_api_helper');
      $this->liftClient = $liftHelper->getLiftClient();
    } catch (AcquiaLiftException $e) {
      drupal_set_message($this->t($e->getMessage()), 'error');
    }
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    /** @var \Drupal\Core\Entity\EntityTypeManager $entity_type_manager */
    $entity_type_manager = $container->get('entity_type.manager');

    return new static($entity_type_manager);
  }

  /**
   * {@inheritdoc}
   */
  public function form(array $form, FormStateInterface $form_state) {
    /** @var \Drupal\acquia_lift\SlotInterface $slot */
    // Drupal provides the entity to us as a class variable. If this is an
    // existing entity, it will be populated with existing values as class
    // variables. If this is a new entity, it will be a new object with the
    // class of our entity. Drupal knows which class to call from the
    // annotation on our Slot class.
    $slot = $this->entity;

    $form['label'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Label'),
      '#description' => $this->t(
        'The administrative name used for this facet.'
      ),
      '#default_value' => $slot->label(),
      '#required' => TRUE,
    ];

    $form['id'] = [
      '#type' => 'machine_name',
      '#default_value' => $slot->id(),
      '#maxlength' => 50,
      '#required' => TRUE,
      '#disabled' => !$slot->isNew(),
      '#machine_name' => [
        'exists' => [$this->slotStorage, 'load'],
        'source' => ['label'],
      ],
    ];

    $form['description'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Description'),
      '#description' => $this->t('Describe the use of this slot.'),
      '#default_value' => $slot->getDescription(),
      '#required' => TRUE,
    ];

    $form['visibility'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Visibility settings'),
    );

    // Get the Slot's visibility options
    $visibility = $slot->getVisibility();

    $form['visibility']['pages'] = [
      '#type' => 'textarea',
      '#title' => $this->t('Slot Pages'),
      '#description' => $this->t(
        'Enter the pages that you wish this slot is actived or deactivated on.'
      ),
      '#default_value' => implode('\n', $visibility->getPages()),
      '#required' => TRUE,
    ];

    $form['visibility']['condition'] = [
      '#type' => 'select',
      '#title' => $this->t('Condition'),
      '#options' => [
        'show' => $this->t('Show on given pages'),
        'hide' => $this->t('Hide on given pages'),
      ],
      '#description' => $this->t(
        'Do you want the listed pages to be shown or hidden on the given pages?'
      ),
      '#default_value' => !empty($visibility) ? $visibility->getCondition(
      ) : 'show',
      '#required' => TRUE,
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    parent::validateForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    parent::submitForm($form, $form_state);

    /** @var \Drupal\acquia_lift\SlotInterface $slot */
    $slot = $this->getEntity();

    $slot->set('label', $form_state->getValue('label'));
    $slot->setDescription($form_state->getValue('description'));

    $visibility = new Visibility();
    $visibility->setCondition($form_state->getValue('condition'));
    $visibility->setPages(explode('\n', $form_state->getValue('pages')));
    $slot->setVisibility($visibility);

  }

  public function save(array $form, FormStateInterface $form_state) {
    /** @var \Drupal\acquia_lift\SlotInterface $slot */
    $slot = $this->getEntity();

    // Only save the index if the form doesn't need to be rebuilt.
    if (!$form_state->isRebuilding()) {
      try {
        // Drupal already populated the form values in the entity object. Each
        // form field was saved as a public variable in the entity class. PHP
        // allows Drupal to do this even if the method is not defined ahead of
        // time.
        $status = $slot->save();

        // Grab the slot in the format that the SDK expects it
        $externalSlot = $slot->getExternalSlot();

        try {
          $externalSlot = $this->liftClient->getSlotManager()->add($externalSlot);
        } catch (\Exception $e) {
          drupal_set_message($this->t($e->getMessage()), 'error');
        }

        // Grab the URL of the new entity. We'll use it in the message.
        $url = $slot->urlInfo();

        // Create an edit link.
        $edit_link = Link::fromTextAndUrl($this->t('Edit'), $url)->toString();

        if ($status == SAVED_UPDATED) {
          // If we edited an existing entity...
          drupal_set_message(
            t('Slot %label has been updated.', ['%label' => $slot->label()])
          );
          $this->logger('contact')->notice(
            'Slot %label has been updated.',
            ['%label' => $slot->label(), 'link' => $edit_link]
          );
        }
        else {
          // If we created a new entity...
          drupal_set_message(
            $this->t(
              'Slot %label has been added.',
              array('%label' => $slot->label())
            )
          );
          $this->logger('contact')->notice(
            'Slot %label has been added.',
            ['%label' => $slot->label(), 'link' => $edit_link]
          );
        }

        // Redirect the user back to the listing route after the save operation.
        $form_state->setRedirect('acquia_lift.slot.overview');
      } catch (AcquiaLiftException $e) {
        $form_state->setRebuild();
        watchdog_exception('acquia_lift', $e);
        drupal_set_message($this->t('The slot could not be saved.'), 'error');
      }
    }
  }
}
