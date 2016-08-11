set( libs
  String
  )
foreach( lib ${libs} )
  add_library( ${lib} STATIC ${lib}.cxx )
  install( TARGETS ${lib} DESTINATION lib )
endforeach( lib )
