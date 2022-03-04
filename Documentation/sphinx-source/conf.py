# conf.py for sphinx
import sys, os

# project specific
texinfo_documents = [
  ('index', 'CanModule', u'Documentation',
   u'Michael Ludwig', 'CanModule', 'CanModule SW abstraction layer',
   'Miscellaneous'),
]
latex_documents = [
  ('index', 'CanModule.tex', u'CanModule Developer Documentation',
   u'Michael Ludwig', 'manual'),
]
man_pages = [
    ('index', 'CanModule', u'Documentation',
     [u'Michael Ludwig'], 1)
]
breathe_projects = { "CanModule": "./doxygen-result/xml" }
breathe_default_project = "CanModule"
project = u'CanModule'
copyright = u'2020, CERN, quasar-team, BE-ICS (Michael Ludwig)'
version = '2.0.16'
release = '2.0.16'
qthelp_namespace = 'CanModule'
htmlhelp_basename = 'CanModule'

# general sphinx
sys.path.append( "/breathe" )
needs_sphinx = '1.8.5'
extensions = ['sphinx.ext.autodoc', 'sphinx.ext.todo', 'sphinx.ext.coverage', 'breathe']
templates_path = ['sphinx_templates']
source_suffix = '.rst'
master_doc = 'index'
language = 'en'
today_fmt = '%B %d, %Y'
exclude_patterns = []
pygments_style = 'sphinx'
qthelp_basename = breathe_default_project
qthelp_theme = 'haiku'
#html_theme = 'nature'
html_theme = 'agogo'

html_theme_options = {}
html_last_updated_fmt = '%d-%b-%Y %H:%M:%S'
html_sidebars = {
   '**': ['globaltoc.html', 'sourcelink.html', 'searchbox.html'],
   'using/windows': ['windowssidebar.html', 'searchbox.html'],
}
html_use_index = True
html_split_index = False
html_show_sourcelink = True
html_show_sphinx = True
html_show_copyright = True
latex_elements = {
'papersize': 'a4paper',
'pointsize': '10pt'
}
man_show_urls = False

