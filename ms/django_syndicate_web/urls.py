from django.conf.urls import patterns, include, url

# Uncomment the next two lines to enable the admin:
# from django.contrib import admin
# admin.autodiscover()

urlpatterns = patterns('',
						url(r'^syn/volume/', include('django_volume.urls')),
						url(r'^syn/', include('django_home.urls')),
                        url(r'^syn', 'django_home.views.home'),

    # Examples:
    # url(r'^$', 'synweb.views.home', name='home'),
    # url(r'^synweb/', include('synweb.foo.urls')),

    # Uncomment the admin/doc line below to enable admin documentation:
    # url(r'^admin/doc/', include('django.contrib.admindocs.urls')),

    # Uncomment the next line to enable the admin:
    # url(r'^admin/', include(admin.site.urls)),
)
