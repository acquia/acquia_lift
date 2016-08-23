<?php

namespace Drupal\acquia_lift;

use Drupal\Core\Config\Entity\ConfigEntityInterface;
use Drupal\Core\Config\Entity\ConfigEntityListBuilder;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Link;
use Drupal\acquia_lift\Entity\Slot;
use Drupal\acquia_lift\SlotInterface;

/**
 * Builds a listing of slot entities.
 */
class SlotListBuilder extends ConfigEntityListBuilder {

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
    public function getDefaultOperations(EntityInterface $entity) {
        $operations = parent::getDefaultOperations($entity);

        if ($entity->access('update') && $entity->hasLinkTemplate('edit-form')) {
            $operations['edit'] = array(
              'title' => $this->t('Edit'),
              'weight' => 10,
              'url' => $entity->toUrl('edit-form'),
            );
        }
        if ($entity->access('delete') && $entity->hasLinkTemplate('delete-form')) {
            $operations['delete'] = array(
              'title' => $this->t('Delete'),
              'weight' => 100,
              'url' => $entity->toUrl('delete-form'),
            );
        }

        return $operations;
    }

    /**
     * {@inheritdoc}
     */
    public function buildHeader() {
        $header = [
          'type' => $this->t('Type'),
          'title' => [
            'data' => $this->t('Title'),
          ],
        ];
        return $header + parent::buildHeader();
    }

    /**
     * {@inheritdoc}
     */
    public function buildRow(EntityInterface $entity) {
        /** @var \Drupal\acquia_lift\SlotInterface $entity */
        $row = parent::buildRow($entity);

        return array(
          'data' => array(
            'type' => array(
              'data' => 'Slot',
              'class' => array('slot-type'),
            ),
            'title' => array(
              'data' => array(
                  '#type' => 'link',
                  '#title' => $entity->label(),
                ) + $entity->toUrl('edit-form')->toRenderArray(),
              'class' => array('acquia-lift-title'),
            ),
            'operations' => $row['operations'],
          ),
          'title' => $this->t('ID: @name', array('@name' => $entity->id())),
          'class' => array('slot'),
        );
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
        uasort($entities, function (ConfigEntityInterface $a, ConfigEntityInterface $b) {
            return strnatcasecmp($a->label(), $b->label());
        });
    }

}
