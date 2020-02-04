library(ggplot2)  # (Wickham, 2009)
library(dplyr)    # (Wickham et al., 2018)
library(cowplot)  # (Wilke, 2018)
library(viridis)  # (Garnier, 2018)

data_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/data"
dump_dir <- "/Users/amlalejini/devo_ws/signalgp-genetic-regulation/experiments/alife-2020-sgp-reg/analysis"
trace_ids <- 100001:100100

max_fit_data_file <- paste(data_dir, "/alt-sig/max_fit_orgs_10000_no_program.csv", sep="")
max_fit_data <- read.csv(max_fit_data_file, na.strings="NONE")
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

for (trace_id in trace_ids) {
  
  org_info <- filter(max_fit_data, SEED==trace_id)
  num_envs <- org_info$NUM_SIGNAL_RESPONSES
  score <- org_info$score
  condition <- org_info$condition
  is_sol <- org_info$solution
  if (is_sol == "0") {
    next
  }
  
  trace_file <- paste(data_dir, "/alt-sig/traces/trace_update-10000_run-id-", trace_id, ".csv", sep="")
  trace_data <- read.csv(trace_file, na.strings="NONE")
  trace_data$similarity_score <- 1 - trace_data$match_score
  trace_data$is_running <- factor(trace_data$is_running, levels=c(0, 1))
  
  trace_data$triggered <- (trace_data$env_signal_closest_match == trace_data$module_id) & (trace_data$cpu_step == "0")
  
  out_name <- paste(dump_dir, "/imgs/traces/trace-id-", trace_id, "-regulator-state.png", sep="")
  ggplot(trace_data, aes(x=module_id, y=time_step, fill=regulator_state)) +
    scale_fill_viridis(option="plasma") +
    geom_tile(color="white", size=0.2, width=1, height=1) +
    geom_tile(data=filter(trace_data, is_running==1), color="black", width=1, height=1, size=0.8) +
    geom_hline(yintercept=filter(trace_data, cpu_step==0)$time_step-0.5, size=1) +
    geom_point(data=filter(trace_data, triggered==TRUE), size=0.4) +
    ggtitle(paste("Condition: ",condition, "; # Envs: ", num_envs, "; Score: ", score, "; Solution? ", is_sol , sep="")) +
    ggsave(out_name, height=8, width=8)
  
  out_name <- paste(dump_dir, "/imgs/traces/trace-id-", trace_id, "-similarity-score.png", sep="")
  ggplot(trace_data, aes(x=module_id, y=time_step, fill=similarity_score)) +
    scale_fill_viridis(option="plasma") +
    geom_tile(color="white", size=0.2, width=1, height=1) +
    geom_tile(data=filter(trace_data, is_running==1), color="black", width=1, height=1, size=0.8) +
    geom_point(data=filter(trace_data, triggered==TRUE), size=0.4) +
    geom_hline(yintercept=filter(trace_data, cpu_step==0)$time_step-0.5, size=1) +
    ggtitle(paste("Condition: ",condition, "; # Envs: ", num_envs, "; Score: ", score, "; Solution? ", is_sol , sep="")) +
    ggsave(out_name, height=8, width=8)
  
  # out_name <- paste(dump_dir, "/imgs/traces/trace-id-", trace_id, "-is-running.png", sep="")
  # ggplot(trace_data, aes(x=module_id, y=time_step, fill=is_running)) +
  #   geom_tile(color="gray") +
  #   geom_hline(yintercept=filter(trace_data, cpu_step==0)$time_step-0.5, size=1) +
  #   geom_point(data=filter(trace_data, triggered==TRUE), size=0.4) +
  #   ggsave(out_name, height=8, width=8)
}

