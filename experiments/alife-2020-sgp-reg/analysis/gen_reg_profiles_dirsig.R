library(ggplot2)  # (Wickham, 2009)
library(dplyr)    # (Wickham et al., 2018)
library(cowplot)  # (Wilke, 2018)
library(viridis)  # (Garnier, 2018)
library(scales)

data_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/data/dir-sig/"
dump_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/analysis/imgs/dir-sig/traces/"
trace_ids <- 101:190


max_fit_data_file <- paste(data_dir, "max_fit_orgs.csv", sep="")
max_fit_data <- read.csv(max_fit_data_file, na.strings="NONE")
max_fit_data <- subset(max_fit_data, select = -c(program))

# Define function to summarize regulation/memory configurations.
get_con <- function(reg, mem) {
  if (reg == "0" && mem == "0") {
    return("none")
  } else if (reg == "0" && mem=="1") {
    return("memory")
  } else if (reg=="1" && mem=="0") {
    return("regulation")
  } else if (reg=="1" && mem=="1") {
    return("both")
  } else {
    return("UNKNOWN")
  }
}
# Specify experimental condition for each datum.
max_fit_data$condition <- mapply(get_con, max_fit_data$USE_FUNC_REGULATION, max_fit_data$USE_GLOBAL_MEMORY)
max_fit_data$condition <- factor(max_fit_data$condition, levels=c("regulation", "memory", "none", "both"))

trace_ids <- 131:160
# trace_id <- 127
# if (TRUE) {
for (trace_id in trace_ids) {
  
  org_info <- filter(max_fit_data, SEED==trace_id)
  num_env_states <- org_info$NUM_ENV_STATES
  num_env_updates <- org_info$NUM_ENV_UPDATES
  score <- org_info$aggregate_score
  condition <- org_info$condition
  is_sol <- org_info$solution
  
  if (is_sol == "0") {
    next
  }
  
  trace_file <- paste(data_dir, "trace_update-1000_run-id-", trace_id, ".csv", sep="")
  trace_data <- read.csv(trace_file, na.strings="NONE")
  trace_data$left_similarity_score <- 1 - trace_data$X0_match_score
  trace_data$right_similarity_score <- 1 - trace_data$X1_match_score
  trace_data$triggered <- (trace_data$is_match_cur_dir=="1") & (trace_data$cpu_step == "0")
  trace_data$is_running <- trace_data$is_running > 0
  # Visualize each environment sequence
  test_ids <- levels(factor(trace_data$test_id))
  for (test_num in test_ids) {
    test_data <- filter(trace_data, test_id==test_num)
    env_id <- test_num
    env_seq <- test_data$env_seq
    
    plot_title <- paste("Condition: ", condition, "; # Env states: ", num_env_states, "; Score: ", score, "; Solution? ", is_sol, "; Test ID: ", env_id, sep="")
    out_name <- paste(dump_dir, "trace-id-", trace_id, "-env-id-", env_id, "-regulator-state.png", sep="")
    ggplot(test_data, aes(x=module_id, y=time_step, fill=regulator_state)) +
      scale_fill_viridis(option="viridis", direction=-1) +
      geom_tile(color="white", size=0.2, width=1, height=1) +
      geom_tile(data=filter(test_data, is_running==TRUE), color="black", width=1, height=1, size=0.8) +
      geom_hline(yintercept=filter(test_data, cpu_step==0)$time_step-0.5, size=1) +
      geom_point(data=filter(test_data, triggered==TRUE), size=0.4) +
      ggtitle(plot_title) +
      ggsave(out_name, height=8, width=8)
    
    out_name <- paste(dump_dir, "trace-id-", trace_id, "-env-id-", env_id, "-similarity-score.png", sep="")
    out_name_left <- paste(dump_dir, "trace-id-", trace_id, "-env-id-", env_id, "-similarity-score-left.png", sep="")
    out_name_right <- paste(dump_dir, "trace-id-", trace_id, "-env-id-", env_id, "-similarity-score-right.png", sep="")
    ggplot(test_data, aes(x=module_id, y=time_step, fill=left_similarity_score)) +
              scale_fill_viridis(option="plasma") +
              geom_tile(color="white", size=0.2, width=1, height=1) +
              geom_tile(data=filter(test_data, is_running==TRUE), color="black", width=1, height=1, size=0.8) +
              geom_point(data=filter(test_data, triggered==TRUE), size=0.4) +
              geom_hline(yintercept=filter(test_data, cpu_step==0)$time_step-0.5, size=1) +
              ggtitle(plot_title) +
              ggsave(out_name_left, width=8, height=8)
    ggplot(test_data, aes(x=module_id, y=time_step, fill=right_similarity_score)) +
              scale_fill_viridis(option="plasma") +
              geom_tile(color="white", size=0.2, width=1, height=1) +
              geom_tile(data=filter(test_data, is_running==TRUE), color="black", width=1, height=1, size=0.8) +
              geom_point(data=filter(test_data, triggered==TRUE), size=0.4) +
              geom_hline(yintercept=filter(test_data, cpu_step==0)$time_step-0.5, size=1) +
              ggtitle(plot_title) +
              ggsave(out_name_right, width=8, height=8)
  }
}

