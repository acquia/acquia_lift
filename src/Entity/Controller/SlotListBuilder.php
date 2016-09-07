<?php

namespace Drupal\acquia_lift\Entity\Controller;

use Drupal\acquia_lift\Lift\APILoader;
use Drupal\Component\Utility\Html;
use Drupal\Core\Config\Entity\ConfigEntityInterface;
use Drupal\Core\Config\Entity\ConfigEntityListBuilder;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityStorageInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Builds a listing of slot entities.
 */
class SlotListBuilder extends ConfigEntityListBuilder {

  /**
   * The Lift API Helper.
   *
   * @var \Acquia\LiftClient\Lift
   */
  protected $liftClient;

  /**
   * The Lift API Loader.
   *
   * @var \Drupal\acquia_lift\Lift\APILoader
   */
  protected $apiLoader;

  /**
   * Constructs an IndexListBuilder object.
   *
   * @param \Drupal\Core\Entity\EntityTypeInterface $entity_type
   *   The entity type definition.
   * @param \Drupal\Core\Entity\EntityStorageInterface $storage
   *   The entity storage class.
   * @param  \Drupal\acquia_lift\Lift\APILoader $api_loader
   *   The Lift API Loader.
   */
  public function __construct(EntityTypeInterface $entity_type, EntityStorageInterface $storage, APILoader $api_loader) {
    parent::__construct($entity_type, $storage);
    $this->apiLoader = $api_loader;
  }

  /**
   * {@inheritdoc}
   */
  public static function createInstance(ContainerInterface $container, EntityTypeInterface $entity_type) {
    $entity_storage = $container->get('entity_type.manager')
      ->getStorage($entity_type->id());

    /** @var \Drupal\acquia_lift\Lift\APILoader $api_loader */
    $api_loader = $container->get('acquia_lift.lift.api_loader');

    return new static(
      $entity_type,
      $entity_storage,
      $api_loader
    );
  }

  /**
   * {@inheritdoc}
   */
  public function load() {
    $entities = parent::load();
    $this->sortAlphabetically($entities);
    return $entities;
  }

  /**
   * {@inheritdoc}
   */
  public function buildHeader() {
    return array(
      'label' => $this->t('Label'),
      'html' => $this->t('Html'),
      'status' => array(
        'data' => $this->t('Status'),
        'class' => array('checkbox'),
      ),

    ) + parent::buildHeader();
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity) {
    /** @var \Drupal\acquia_lift\SlotInterface $entity */
    $row = parent::buildRow($entity);

    $status = $entity->status();
    $status_label = $status ? $this->t('Enabled') : $this->t('Disabled');

    // Get our liftClient
    $liftClient = NULL;
    try {
      $liftClient = $this->apiLoader->getLiftClient();
      // Verify if we have a connection to the Decision API.
      if (!isset($liftClient) || !$liftClient->ping()) {
        $status = FALSE;
        $status_label = $this->t('Unavailable');
      }
    } catch (\Exception $e) {
      $status = FALSE;
      $status_label = $e->getMessage();
    }

    if ($liftClient instanceof \Acquia\LiftClient\Lift) {
      // Verify if the slot is available in the Decision API.
      try {
        $liftClient->getSlotManager()->get($entity->id());
      } catch (\Exception $e) {
        $status = FALSE;
        $status_label = $e->getMessage();
      }
    }

    $status_icon = array(
      '#theme' => 'image',
      '#uri' => $status ? 'core/misc/icons/73b355/check.svg' : 'core/misc/icons/e32700/error.svg',
      '#width' => 18,
      '#height' => 18,
      '#alt' => $status_label,
      '#title' => $status_label,
    );

    $row = array(
      'data' => array(
        'label' => array(
          'data' => array(
            '#markup' => $entity->label(),
            '#suffix' => '<div>' . $entity->getDescription() . '</div>',
          ),
          'class' => array('acquia-lift-title'),
        ),
        'html' => array(
          'data' => $entity->getHtml(),
          'class' => array('acquia-lift-slot-html'),
        ),
        'status' => array(
          'data' => $status_icon,
          'class' => array('checkbox'),
        ),
        'operations' => $row['operations'],
      ),
      'title' => $this->t('ID: @name', array('@name' => $entity->id())),
      'class' => array(
        Html::cleanCssIdentifier(
          $entity->getEntityTypeId() . '-' . $entity->id()
        ),
        $status ? 'acquia-lift-slot-enabled' : 'acquia-lift-slot-disabled'
      )
    );

    if (!$status) {
      $row['class'][] = 'color-error';
    }

    return $row;
  }

  /**
   * Sorts an array of entities alphabetically.
   *
   * Will preserve the key/value association of the array.
   *
   * @param \Drupal\Core\Config\Entity\ConfigEntityInterface[] $entities
   *   An array of config entities.
   */
  protected function sortAlphabetically(array &$entities) {
    uasort(
      $entities,
      function (ConfigEntityInterface $a, ConfigEntityInterface $b) {
        return strnatcasecmp($a->label(), $b->label());
      }
    );
  }

  /**
   * Adds some descriptive text to our entity list.
   *
   * Typically, there's no need to override render(). You may wish to do so,
   * however, if you want to add markup before or after the table.
   *
   * @return array
   *   Renderable array.
   */
  public function render() {
    $build['description'] = array(
      '#markup' => "<p>" . $this->t("This lists all the slots that were created by Drupal and synced up to the Acquia Lift Service. This does NOT list Slots created in the Acquia Lift Experience builder.") . "</p>",
    );
    $build[] = parent::render();
    return $build;
  }

}
