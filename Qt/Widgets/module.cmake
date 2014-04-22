if (PARAVIEW_ENABLE_QT_GUI OR PARAVIEW_ENABLE_QT_SUPPORT)
  vtk_module(pqWidgets
    GROUPS
      Qt
      ParaViewQt
    DEPENDS
      vtkqttesting
      vtkPVServerManagerCore
      vtkGUISupportQt
    EXCLUDE_FROM_WRAPPING
    TEST_LABELS
      PARAVIEW
  )
endif ()
