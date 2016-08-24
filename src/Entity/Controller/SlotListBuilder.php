<?php

namespace Drupal\acquia_lift\Entity\Controller;

use Drupal\Component\Utility\Html;
use Drupal\Core\Block\BlockManagerInterface;
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
   * The block manager.
   *
   * @var \Drupal\Core\Block\BlockManagerInterface
   */
  protected $blockManager;

  /**
   * Constructs an IndexListBuilder object.
   *
   * @param \Drupal\Core\Entity\EntityTypeInterface $entity_type
   *   The entity type definition.
   * @param \Drupal\Core\Entity\EntityStorageInterface $storage
   *   The entity storage class.
   * @param  \Drupal\Core\Block\BlockManagerInterface
   *   The Block Manager.
   */
  public function __construct(EntityTypeInterface $entity_type, EntityStorageInterface $storage, BlockManagerInterface $block_manager) {
    parent::__construct($entity_type, $storage);
    $this->blockManager = $block_manager;
  }

  /**
   * {@inheritdoc}
   */
  public static function createInstance(ContainerInterface $container, EntityTypeInterface $entity_type) {
    /** @var \Drupal\Core\Block\BlockManagerInterface $block_manager */
    $block_manager = $container->get('plugin.manager.block');

    $entity_storage = $container->get('entity_type.manager')
      ->getStorage($entity_type->id());

    return new static(
      $entity_type,
      $entity_storage,
      $block_manager
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
      'type' => $this->t('Type'),
      'label' => $this->t('Label'),
      'status' => array(
        'data' => $this->t('Status'),
        'class' => array('checkbox'),
      ),
      'html' => $this->t('Html'),

    ) + parent::buildHeader();
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity) {
    /** @var \Drupal\acquia_lift\SlotInterface $entity */
    $row = parent::buildRow($entity);

    $status = $entity->status();
    $status_server = TRUE;
    $status_label = $status ? $this->t('Enabled') : $this->t('Disabled');

    // Verify if entity is available in the decision API.
    /*if ($entity->status() && !$entity->isAvailable()) {
        $status = FALSE;
        $status_server = FALSE;
        $status_label = $this->t('Unavailable');
    }*/

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
        'type' => array(
          'data' => $this->t('Slot for Drupal Block'),
          'class' => array('acquia-lift-slot-type'),
        ),
        'label' => array(
          'data' => array(
              '#markup' => $entity->label(),
              '#suffix' => '<div>' . $entity->getDescription() . '</div>',
            ),
          'class' => array('acquia-lift-title'),
        ),
        'status' => array(
          'data' => $status_icon,
          'class' => array('checkbox'),
        ),
        'html' => array(
          'data' => $entity->getHtml(),
          'class' => array('acquia-lift-slot-html'),
        ),
        'operations' => $row['operations'],
      ),
      'title' => $this->t('ID: @name', array('@name' => $entity->id())),
      'class' => array(
        Html::cleanCssIdentifier(
          $entity->getEntityTypeId() . '-' . $entity->id()
        ),
        $status ? 'acquia-lift-slot-enabled' : 'acquia-lift-slot-enabled'
      )
    );

    if (!$status_server) {
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

}
