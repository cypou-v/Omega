apps += terminal::App
app_headers += apps/terminal/app.h

app_sterminal_src = $(addprefix apps/terminal/,\
  app.cpp \
)

apps_src += $(app_sterminal_src)

app_images += apps/terminal/terminal_icon.png

i18n_files += $(call i18n_without_universal_for,terminal/base)

$(eval $(call depends_on_image,apps/terminal/app.cpp,apps/terminal/terminal_icon.png))