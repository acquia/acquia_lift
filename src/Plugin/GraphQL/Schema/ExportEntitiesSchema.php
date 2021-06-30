<?php

namespace Drupal\acquia_perz\Plugin\GraphQL\Schema;

use Drupal\graphql\GraphQL\ResolverBuilder;
use Drupal\graphql\GraphQL\ResolverRegistry;
use Drupal\graphql\Plugin\GraphQL\Schema\SdlSchemaPluginBase;

/**
 * @Schema(
 *   id = "export_entities",
 *   name = "Export entities schema"
 * )
 */
class ExportEntitiesSchema extends SdlSchemaPluginBase {

  /**
   * {@inheritdoc}
   */
  public function getResolverRegistry() {
    $builder = new ResolverBuilder();
    $registry = new ResolverRegistry();

    $this->addQueryFields($registry, $builder);

    return $registry;
  }

  /**
   * @param \Drupal\graphql\GraphQL\ResolverRegistry $registry
   * @param \Drupal\graphql\GraphQL\ResolverBuilder $builder
   */
  protected function addQueryFields(ResolverRegistry $registry, ResolverBuilder $builder) {
    $registry->addFieldResolver('Query', 'entity_variations',
      $builder->produce('query_entity_variations')
        ->map('entity_type_id', $builder->fromArgument('entity_type_id'))
        ->map('entity_uuid', $builder->fromArgument('entity_uuid'))
    );

    $registry->addFieldResolver('Query', 'discover_entities',
      $builder->produce('query_discover_entities')
        ->map('page', $builder->fromArgument('page'))
    );
  }

}
