#include "cameff/cameff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *p){fprintf(stderr,"Usage: %s analyze --catalog FILE --cutoff ISO --lat N --lon N --radius-km N [--lookback-days N] [--config FILE]\n",p);} 
static const char *arg(int argc,char **argv,const char *name){int i;for(i=2;i+1<argc;i++)if(strcmp(argv[i],name)==0)return argv[i+1];return NULL;}
int main(int argc,char **argv){cameff_catalog_t c;cameff_config_t cfg;cameff_analysis_window_t w;cameff_signal_frame_t f;cameff_assessment_t a;char err[256]={0};const char *cat,*cut,*lat,*lon,*rad,*look,*conf;unsigned i;cameff_status_t st;
 if(argc<2||strcmp(argv[1],"analyze")!=0){usage(argv[0]);return 2;} cat=arg(argc,argv,"--catalog");cut=arg(argc,argv,"--cutoff");lat=arg(argc,argv,"--lat");lon=arg(argc,argv,"--lon");rad=arg(argc,argv,"--radius-km");look=arg(argc,argv,"--lookback-days");conf=arg(argc,argv,"--config");if(!cat||!cut||!lat||!lon||!rad){usage(argv[0]);return 2;}
 cameff_config_default(&cfg);if(conf&&cameff_config_load(conf,&cfg,err,sizeof err)!=CAMEFF_STATUS_OK){fprintf(stderr,"config: %s\n",err);return 1;}cameff_catalog_init(&c);if(cameff_catalog_load_csv(cat,&c,err,sizeof err)!=CAMEFF_STATUS_OK){fprintf(stderr,"catalog: %s\n",err);return 1;}
 if(cameff_parse_iso8601_utc(cut,&w.cutoff_epoch_seconds)!=CAMEFF_STATUS_OK){fprintf(stderr,"invalid cutoff\n");return 1;}w.center_latitude_deg=strtod(lat,NULL);w.center_longitude_deg=strtod(lon,NULL);w.radius_km=strtod(rad,NULL);w.lookback_days=look?(unsigned)strtoul(look,NULL,10):30U;
 st=cameff_extract_signal_frame(&c,&w,&cfg,&f);a=cameff_assess(&f,&cfg);printf("CAMEFF b1.0.0 Milestone 1\nselected_events=%zu\nstatus=%d\n",f.selected_event_count,(int)st);for(i=0;i<CAMEFF_SIGNAL_COUNT;i++)printf("signal.%s=%.6f confidence=%.6f available=%d\n",cameff_signal_name((cameff_signal_id_t)i),f.signals[i].value,f.signals[i].confidence,f.signals[i].available);printf("evidence_score=%.6f\nconfidence=%.6f\ndecision=%s\n",a.evidence_score,a.confidence,cameff_decision_level_name(a.level));cameff_catalog_free(&c);return 0;}
