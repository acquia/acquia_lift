<?php

namespace Drupal\Tests\acquia_lift\Unit\Traits;

use Drupal\Core\Language\LanguageInterface;
use Drupal\field\Entity\FieldConfig;
use Drupal\field\Entity\FieldStorageConfig;
use Drupal\taxonomy\Entity\Term;
use Drupal\taxonomy\Entity\Vocabulary;

/**
 * Fixtures Data Trait.
 */
trait FixturesDataTrait {

  /**
   * Returns a new vocabulary with random properties.
   */
  private function createVocabulary() {
    // Create a vocabulary.
    $vocabulary = Vocabulary::create([
      'name' => $this->randomMachineName(),
      'description' => $this->randomMachineName(),
      'vid' => mb_strtolower($this->randomMachineName()),
      'langcode' => LanguageInterface::LANGCODE_NOT_SPECIFIED,
      'weight' => mt_rand(0, 10),
    ]);
    $vocabulary->save();
    return $vocabulary;
  }

  /**
   * Returns a new term with random properties in vocabulary $vid.
   *
   * @param \Drupal\taxonomy\Entity\Vocabulary $vocabulary
   *   The vocabulary object.
   * @param array $values
   *   (optional) An array of values to set, keyed by property name. If the
   *   entity type has bundles, the bundle key has to be specified.
   *
   * @return \Drupal\taxonomy\Entity\Term
   *   The new taxonomy term object.
   */
  private function createTerm(Vocabulary $vocabulary, array $values = []) {
    $filter_formats = filter_formats();
    $format = array_pop($filter_formats);
    $term = Term::create($values + [
      'name' => $this->randomMachineName(),
      'description' => [
        'value' => $this->randomMachineName(),
        // Use the first available text format.
        'format' => $format->id(),
      ],
      'vid' => $vocabulary->id(),
      'langcode' => LanguageInterface::LANGCODE_NOT_SPECIFIED,
    ]);
    $term->save();
    return $term;
  }

  /**
   * Creates a field with storage.
   *
   * @param string $field_name
   *   The field name.
   * @param string $entity_type
   *   The field entity type.
   * @param int $bundle
   *   The field's bundle.
   * @param array $target_bundles
   *   The field's target bundles.
   * @param array $storage_settings
   *   The field storage's settings.
   * @param string $storage_type
   *   The field storage's type.
   */
  private function createFieldWithStorage($field_name, $entity_type, $bundle, array $target_bundles, array $storage_settings, $storage_type) {
    $field_storage = FieldStorageConfig::create([
      'field_name' => $field_name,
      'entity_type' => $entity_type,
      'translatable' => FALSE,
      'settings' => $storage_settings,
      'type' => $storage_type,
      'cardinality' => 1,
    ]);
    $field_storage->save();
    $field = FieldConfig::create([
      'field_storage' => $field_storage,
      'entity_type' => $entity_type,
      'bundle' => $bundle,
      'settings' => [
        'handler' => 'default',
        'handler_settings' => [
          'target_bundles' => $target_bundles,
        ],
      ],
    ]);
    $field->save();
  }

}
